#include "modularcore.h"
#include "moduleplugin.h"
#include "module-defines.h"
#include "module.h"

#ifndef LEGACY_QT
#include <QRegularExpression>
#endif
#include <QDomDocument>
#include <QFileInfo>
#include <QDir>

#include <QDebug>

const Module::Version ModularCore::coreVersion() {
    return (Module::Version){(quint8)RAW_VER_MAJ, (quint8)RAW_VER_MIN,
#ifdef RAW_GIT_REVISION
                (quint32)RAW_GIT_REVISION
#else
                (quint32)0
#endif
            , QString(
#ifdef GIT_BRANCH
                GIT_BRANCH
#else
                "unknown"
#endif
                ), QString(
#ifdef GIT_SOURCE
                GIT_SOURCE
#else
                "unknown"
#endif
                )};
}

QVariant nodeToVariant(QDomNode el) {
    if(el.hasAttributes() || el.hasChildNodes()) {
        QVariantMap map;
        if(el.attributes().length()) {
#if LEGACY_QT
            for(uint i=0; i<el.attributes().length(); i++) {
#else
            for(int i=0; i<el.attributes().length(); i++) {
#endif
                QString value = nodeToVariant(el.attributes().item(i)).toString();
                map.insert(el.attributes().item(i).nodeName(), value.isEmpty() ? "yes" : value);
            }

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
    TypeInfo typeInfo = _types.value(type);
    if(typeInfo.first.isEmpty())
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
                typeInfo.first + '/' +
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
                                        if(!typeInfo.second.isEmpty()) {
                                            qDebug() << "Loading dependancies..." << typeInfo.second;
                                            foreach(QVariant dep, metaData.value(typeInfo.second).toList())
                                                deps << loadModuleByDefinition(dep.toMap());
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
                library.close();

                module = Module::create(name, type, deps, libPath, this);
                moduleVerify(module);
            } else
                throw QString("Cannot locate requested module `%1` of type `%2`.").arg(name).arg(type);
        }
    }

    return module;
}

inline bool isBool(QString s, bool emptyTrue =true) {
    if(s.isEmpty())
        return emptyTrue;
    return (s = s.toLower()) == "yes" || s == "no";
}

Module::Ref ModularCore::loadModuleByDefinition(QVariantMap def) {
    Module::Ref module;
    if(!isBool(def.value("required").toString()))
        try {
            module = loadModuleByDefinition(def);
        } catch(...) {}
    else if(def.contains("library")) {
        QString libPath = def.value("library").toString();
        module = _knownModules.value(libPath).toStrongRef();
        if(!module) {
            module = Module::create(def.value("class", "Unknown").toString(), def.value("type", "Library").toString(), libPath, this);
            _knownModules.insert(libPath, module);
        }
    } else if(def.contains("class"))
        module = loadModule(def.value("class").toString(), def.value("type", "Module").toString());
    else
        throw "Expected either library or class attribute.";
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
    if(!flags.testFlag(LazyLoadSymbols))
        hints |= QLibrary::ResolveAllSymbolsHint;
    if(flags.testFlag(ExportSymbols))
        hints |= QLibrary::ExportExternalSymbolsHint;

    if(_lib.isLoaded()) {
        if(_lib.loadHints() == hints)
            return;

        unload();
    }

    qDebug() << libraryFile();
    _lib.setLoadHints(hints);
    if(!_lib.load()) {
#ifdef Q_OS_UNIX
        static QRegExp missingDep("Cannot load library .+: \\((.+): (.+): (.+)\\)\\s*");
        if(missingDep.exactMatch(_lib.errorString())) {
            if(missingDep.cap(2) == "undefined symbol")
                throw QString("Symbol `%1` could not be resolved, `%2` may be a different version than what this module was compiled against.").arg(missingDep.cap(3)).arg(missingDep.cap(1));
            else if(missingDep.cap(2) == "cannot open shared object file") {
                if(missingDep.cap(3) == "No such file or directory")
                    throw QString("Required library `%1` missing, install it and try again.").arg(missingDep.cap(1));
                else
                    throw QString("Dependancy failed to load: `%1`: %2.").arg(missingDep.cap(1)).arg(missingDep.cap(3));
            } else
                    throw QString("%1 (%2): %3.").arg(missingDep.cap(2)).arg(missingDep.cap(3)).arg(missingDep.cap(1));

        }
#endif
        throw _lib.errorString();
    }

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

void Module::processInfoStrings(LoadFlags flags) {
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
        _version.major = versionReg.cap(1).toInt();
        _version.minor = versionReg.cap(2).toInt();
        if(!versionReg.cap(4).isEmpty())
            _version.revision = versionReg.cap(4).toInt();
        if(!versionReg.cap(6).isEmpty())
            _version.branch = versionReg.cap(6);
    }
    _version.source = _info.at(4);

    qDebug() << authorsString();
    QHash<QString, AuthorRef> _authors;
#ifdef LEGACY_QT
#else
    QRegularExpression expr("([^<]+?)(<(.+?)>)[\\s,]?");
    QRegularExpressionMatchIterator iterator = expr.globalMatch(authorsString());
    while(iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString email = match.captured(3);
        if(email.isEmpty())
            email = match.captured(1);
        AuthorRef author = _authors.value(email);
        if(!author) {
            author = AuthorRef(new Author());
            _authors.insert(email, author);
            _authorList << author;
        }
        if(!author->name.isEmpty())
            author->altNames << author->name;
        author->name = match.captured(1);
        author->email = email;
    }
#endif
    foreach(AuthorRef ref, _authorList)
        qDebug() << ref->name << ref->email << ref->altNames;

    if(_core && _self)
        _core->moduleInformation(_self.toStrongRef());
}

void Module::processEntries(const ModuleEntryList &entries) {
    qDebug() << "Processing module entries...";

    if(entries.isEmpty())
        throw "No entries found...";

    bool foundQtLibVersion = false;
    bool foundVerifyString = false;
    const char* verifyString = _core ? _core->verificationString() : 0;

    foreach(ModuleEntry entry, entries) {
        if(!foundQtLibVersion) {
            switch(entry.first) {
                case QtVersionType:
                {
                    if(!foundVerifyString && verifyString && _loadFlags.testFlag(StrictVerify))
                        throw "Verification string required but module does not supply one.";

                    if(foundQtLibVersion)
                        throw "QtVersion declared multiple times.";

                    if(((quintptr)entry.second ^ 0x00ffff) != ((quintptr)QT_VERSION ^ 0x00ffff))
                        throw QString("Qt Version Mismatch. Library built against 0x%1, but this application uses 0x%2.").arg((quintptr)entry.second, 1, 16).arg((quintptr)QT_VERSION, 1, 16);

                    if(((quintptr)entry.second & 0x00ffff) > ((quintptr)QT_VERSION & 0x00ffff))
                        throw QString("Qt Version Incompatible. Module uses a newer Qt Version (0x%1) than this application (0x%2).").arg((quintptr)entry.second, 1, 16).arg((quintptr)QT_VERSION, 1, 16);

                    foundQtLibVersion = true;
                    continue;
                }

                case GenericVerifyStringType:
                {
                    if(_loadFlags.testFlag(StrictVerify) && !_loadFlags.testFlag(AllowGenericLibs))
                        throw "Cannot load a generic library in strict verification mode, without the AllowGenericLibs flag.";

                    if(foundQtLibVersion)
                        throw "Verifcation string must appear before Qt Version in entry list.";

                    if(strcmp((const char*)entry.second, __MODULE_GENERIC_VERIFICATION_STRING) != 0)
                        throw "Generic Library verification string mismatch.";

                    foundVerifyString = true;
                    continue;
                }

                case VerifyStringType:
                {
                    if(foundQtLibVersion)
                        throw "Verifcation string must appear before Qt Version in entry list.";

                    if(verifyString) {
                        if(strcmp(verifyString, (const char*)entry.second) != 0)
                            throw "Verification string mismatch.";

                        foundVerifyString = true;
                        continue;
                    } else
                        throw "Module provides a verification string, but this application was not built with one.";

                }

                default:
                {
                    throw "Expected verification string or QT Version entry.";
                }
            }

            continue;
        }

        switch(entry.first) {
            case InfoStringType:
                _info << QString::fromLocal8Bit((const char*)entry.second);
                continue;

            case MetaObjectType:
            {
                const QMetaObject* metaObject = (const QMetaObject*)entry.second;
                _plugins.insert(metaObject->className(), metaObject);
                continue;
            }

            case VerifyStringType:
                if(foundVerifyString)
                    throw "Only one verification string is allowed per module.";
                if(foundQtLibVersion)
                    throw "Verification String must appear before Qt Version in entry list.";
                else
                    throw "Verification string must appear first in the entry list.";

            case QtVersionType:
                if(foundQtLibVersion)
                    throw "Only one Qt Version entry is allowed per module.";
                if(foundVerifyString)
                    throw "Qt Version must appear immidiately after the .";
                else
                    throw "Qt Version must be first in the entry list, unless a verification string entry exists.";

            case DataEntryType:
            {
                ModuleData* data = (ModuleData*)entry.second;

                DataMap::Iterator entry = _data.find(data->second.type());
                if(entry == _data.end())
                    entry = _data.insert(data->second.type(), DataIndexList());

                entry.value().insert(data->first, data->second);
                continue;
            }

            default:
                throw QString("Unknown entry type `%1`").arg(entry.first);

        }
    }

    processInfoStrings(_loadFlags);
    qDebug() << "Data detected" << _data;
    qDebug() << "Plugins detected" << _plugins.keys();
}

void ModularCore::moduleEntries(const Module::Ref module, const ModuleEntryList &entries) {
    module->processEntries(entries);
}

void Module::loadEntryPoints(LoadFlags flags) {
    _loadFlags = flags;

    QString symbol = QString("%1EntryList").arg(_entryBaseName);
    {
        QDebug debug(QtDebugMsg);
        debug << "Resolving" << symbol << "...";
        EntryList entryList = (EntryList)_lib.resolve(symbol.toLocal8Bit().data());
        if(entryList) {
            ModuleEntryList entries = entryList();
            debug << "Found!";

            if(_core)
                _core->moduleEntries(this->pointer(), entries);
            else
                processEntries(entries);

            return;
        } else
            debug << "Not found.";
    }
    symbol = QString("%1Information").arg(_entryBaseName);
    {
        QDebug debug(QtDebugMsg);
        debug << "Resolving" << symbol << "...";
        Information info = (Information)_lib.resolve(symbol.toLocal8Bit().data());
        if(info) {
            _info = info();
            processInfoStrings(flags);
        } else if(flags.testFlag(StrictVerify))
            throw "Missing information and entrylist entry points.";
        else {
            _info = QStringList() << "GenericLibrary" << "Unknown" << "Unknown" << "Unknown" << "Unknown";
            debug << "Not found.";
        }
    }
    symbol = QString("%1Constructors").arg(_entryBaseName);
    {
        QDebug debug(QtDebugMsg);
        debug << "Resolving" << symbol << "...";
        Constructors constructors = (Constructors)_lib.resolve(symbol.toLocal8Bit().data());
        if(constructors) {
            foreach(const QMetaObject* metaObject, constructors())
                _plugins.insert(metaObject->className(), metaObject);
            qDebug() << "Plugins detected" << _plugins.keys();
        } else
            debug << "Not found.";
    }

    if(_core && _self)
        _core->moduleVerify(_self.toStrongRef());
}

QObject* Module::createInstance(const QMetaObject* metaObject, QVariantList args) {
    QGenericArgument val[10];
    if(args.size() > 9)
        throw "Cannot handle more than 9 arguments";

    for(int i=0; i<args.size(); i++)
        val[i] = QGenericArgument(args[i].typeName(), (const void*)args[i].data());

    if(!metaObject->constructorCount())
        throw "No invokable constructors.";

    qDebug() << "Constructing" << metaObject->className() << args;
    QObject* obj = metaObject->newInstance(val[0], val[1], val[2], val[3], val[4],
                                            val[5], val[6], val[7], val[8], val[9]);
    if(obj) {
        ModulePlugin* plugin = (ModulePlugin*)ModulePlugin::staticMetaObject.cast(obj);
        if(!plugin) {
            delete obj;
            throw "Returned object is not a valid ModulePlugin";
        }
        plugin->_provider = _self.toStrongRef();
        return plugin;
    }
    return 0;
}

Module::WeakMap ModularCore::_knownModules;
