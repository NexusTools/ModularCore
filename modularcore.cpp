#include "modularcore.h"
#include "moduleplugin.h"
#include "module.h"

#include <QDomDocument>
#include <QFileInfo>
#include <QDir>

#include <QDebug>

QVariant nodeToVariant(QDomNode el) {
    if(el.hasAttributes() || el.hasChildNodes()) {
        QVariantMap map;
        if(el.attributes().length()) {
#if LEGACY_QT
            for(uint i=0; i<el.attributes().length(); i++)
#else
            for(int i=0; i<el.attributes().length(); i++)
#endif
                map.insert(el.attributes().item(i).nodeName(), nodeToVariant(el.attributes().item(i)));

        } else if(el.nodeName().endsWith('s')) {
            QVariantList list;
            QString listNodeName = el.nodeName().mid(0, el.nodeName().length()-1);
#if LEGACY_QT
            for(uint i=0; i<el.childNodes().length(); i++) {
#else
            for(int i=0; i<el.childNodes().length(); i++) {
#endif
                if(el.childNodes().at(i).nodeName() != listNodeName) {
                    list.clear();
                    break;
                }
                list.append(nodeToVariant(el.childNodes().at(i)));
            }

            if(!list.isEmpty())
                return list;
        }

#if LEGACY_QT
        for(uint i=0; i<el.childNodes().length(); i++)
#else
        for(int i=0; i<el.childNodes().length(); i++)
#endif
            map.insert(el.childNodes().at(i).nodeName(), nodeToVariant(el.childNodes().at(i)));

        if(map.size() == 1 && map.contains("#text"))
            return map.value("#text");
        return map;
    } else
        return el.nodeValue();
}

Module::Ref ModularCore::loadModule(QString name, QString type) {
    QString path = _types.value(type);
    if(path.isEmpty())
        throw QString("No type named `%1` registered.").arg(type);

    Module::Ref module = _modules.value(type).value(name).toStrongRef();
    if(!module) {
#ifdef IDE_MODE
        static QString basePath = QFileInfo(QDir::currentPath()).dir().path() + '/';
#endif
        QString libPath =
#ifdef IDE_MODE
                basePath +
#endif
                path + '/' +
#ifdef IDE_MODE
                name + '/' +
#ifdef Q_OS_WIN
#ifdef DEBUG_MODE
                "debug/" +
#else
                "release/" +
#endif
#else
                "lib" +
#endif
#endif
                name +
#ifdef IDE_MODE
#ifdef Q_OS_WIN
                "0" +
#endif
#endif
                '.' +
#ifdef Q_OS_UNIX
                "so";
#else
                "dll";
#endif

        module = _knownModules.value(libPath).toStrongRef();
        if(!module) {
            qDebug() << "Loading" << type << name;
            qDebug() << libPath;

            QFile library(libPath);
            if(library.open(QFile::ReadOnly)) {
                QVariantMap metaData;
                Module::List deps;

                int pos;
                QByteArray buffer;
                while(!library.atEnd()) {
                    if(buffer.length() >= 200)
                        buffer = buffer.mid(100);
                    buffer += library.read(100);
                    if((pos = buffer.indexOf(QString("<%1>").arg(libraryName()))) > -1) {
                        bool ok;
                        quint16 size;
                        size = buffer.mid(pos-4, 4).toHex().toInt(&ok, 16);
                        if(!ok)
                            throw "MetaData size corrupt.";

                        buffer = buffer.mid(pos);
                        size -= buffer.length();
                        while(size > 0) {
                            if(library.atEnd())
                                throw "EOF while parsing metadata.";

                            QByteArray newData = library.read(size);
                            size -= newData.length();
                            buffer += newData;
                        }

                        QDomDocument dom;
                        if(dom.setContent(buffer)) {
                            QDomNodeList children = dom.childNodes();
#ifdef LEGACY_QT
                            for(uint i=0; i<children.length(); i++){
#else
                            for(int i=0; i<children.length(); i++){
#endif
                                QDomNode node = children.at(i);
                                if(node.nodeName() == libraryName()) {
                                    metaData = nodeToVariant(node).toMap();
                                    if(!metaData.isEmpty()) {
                                        qDebug() << metaData;
                                        deps = moduleMetaData(metaData);
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
                library.close();

                module = Module::Ref(new Module(name, type, libPath, this));
                module->_self = module.toWeakRef();
                module->_deps = deps;
                moduleVerify(module);
            } else
                throw QString("Cannot locate requested module `%1` of type `%2`.").arg(name).arg(type);
        }
    }

    return module;
}

ModulePlugin* Module::createPlugin(QString type, QVariantList args) {
    const QMetaObject* p = plugin(type);
    if(!isPluginCompatible<ModulePlugin>(p))
        throw "Object is not a valid ModulePlugin";

    return (ModulePlugin*)createInstance(p, args);
}

void Module::load(LoadFlags flags) {
    QLibrary::LoadHints hints;
    if(flags.testFlag(ExportSymbols))
        hints |= QLibrary::ExportExternalSymbolsHint;

    if(_lib.isLoaded()) {
        if(_lib.loadHints() == hints)
            return;

        unload();
    }

    _lib.setLoadHints(hints);
    if(!_lib.load())
        throw _lib.errorString();

    loadEntryPoints(flags);

    if(_core && _self)
        _core->moduleLoaded(_self.toStrongRef());
}

void Module::unload() {
    if(!_lib.isLoaded())
        return;

    if(!_lib.unload())
        throw _lib.errorString();

    if(_core && _self)
        _core->moduleUnloaded(_self.toStrongRef());
}

void Module::loadEntryPoints(LoadFlags flags) {
    QString symbol = QString("%1Information").arg(_entryBaseName);
    {
        qDebug() << "Resolving" << symbol;
        Information info = (Information)_lib.resolve(symbol.toLocal8Bit().data());
        if(info) {
            _info = info();
            qDebug() << _info;
            if(flags.testFlag(LooseVerify) |
                    flags.testFlag(StrictVerify)) {
                qDebug() << "Verifying information strings...";
                if(_info.isEmpty())
                    throw "Information entry point returned no data.";
                if(_core) {
                    if(!flags.testFlag(IgnoreLibraryName) &&
                            _info.first() != _core->libraryName())
                        throw "Library name mismatch.";
                    if(flags.testFlag(LooseVerify) &&
                            _info.size() < _core->_infoKeys.size())
                        throw "Number of information entries is less than known information keys.";
                    else if(flags.testFlag(StrictVerify) &&
                                _info.size() != _core->_infoKeys.size())
                        throw "Number of information entries does not match known information keys.";
                } else if(!flags.testFlag(StrictVerify))
                    throw "Strict verification requires the module to have a valid ModularCore associated.";
            }

            qDebug() << "Parsing information strings...";
            static QRegExp versionReg("(\\d+)\\.(\\d+)(\\.(\\d+))?( \\((.+)\\))?");
            if(versionReg.exactMatch(version())) {
                _versionParts[0] = versionReg.cap(1).toInt();
                _versionParts[1] = versionReg.cap(2).toInt();
                if(!versionReg.cap(4).isEmpty())
                    _versionParts[2] = versionReg.cap(4).toInt();
                if(!versionReg.cap(6).isEmpty())
                    _branch = versionReg.cap(6);
            }

            if(_core && _self)
                _core->moduleInformation(_self.toStrongRef());
        } else if(flags.testFlag(StrictVerify))
            throw "Missing information entry point.";
        else
            _info = QStringList() << "GenericLibrary" << "Unknown" << "Unknown" << "Unknown" << "Unknown";
    }
    symbol = QString("%1Constructors").arg(_entryBaseName);
    {
        qDebug() << "Resolving" << symbol;
        Constructors constructors = (Constructors)_lib.resolve(symbol.toLocal8Bit().data());
        if(constructors) {
            foreach(const QMetaObject* metaObject, constructors())
                _plugins.insert(metaObject->className(), metaObject);
            qDebug() << "Plugins detected" << _plugins.keys();
        }
    }

    if(_core && _self)
        _core->moduleVerify(_self.toStrongRef());
}

QObject* Module::createInstance(const QMetaObject* metaObject, QVariantList args) {
    QGenericArgument val[10];
    if(args.size() > 10)
        throw "Cannot handle more than 10 arguments";

    for(int i=0; i<args.size(); i++)
        val[i] = QGenericArgument(args[i].typeName(), (const void*)args[i].data());

    if(!metaObject->constructorCount())
        throw "No invokable constructors.";

    qDebug() << "Constructing" << metaObject->className() << args;
    QObject* obj = metaObject->newInstance(val[0], val[1], val[2], val[3], val[4],
                                            val[5], val[6], val[7], val[8], val[9]);
    qDebug() << obj;
    if(obj) {
        ModulePlugin* module = (ModulePlugin*)ModulePlugin::staticMetaObject.cast(obj);
        if(!module) {
            delete obj;
            throw "Returned object is not a valid ModulePlugin";
        }
        return module;
    }
    return 0;
}

Module::WeakMap ModularCore::_knownModules;
