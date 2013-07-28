#include "modularcore.h"
#include "module.h"

#include <QFileInfo>
#include <QDir>

#include <QDebug>

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

                        qDebug() << buffer;
                        //metaData = NexusConfig::parse(buffer, NexusConfig::XmlFormat, "CoordinatorLibrary").toMap();
                        break;
                    }
                }
                library.close();

                Module::Ref module(new Module(name, type, libPath, this));
                module->_self = module.toWeakRef();
                moduleMetaData(module, metaData);
                return module;
            } else
                throw QString("Cannot locate requested module `%1` of type `%2`.").arg(name).arg(type);
        }
    }

    return module;
}

ModulePlugin* Module::createPlugin(QString, QVariantList) {
    return 0;
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
                if(_info.isEmpty())
                    throw "Information entry point returned no data.";
                if(_core) {
                    if(_info.first() != _core->libraryName())
                        throw "Library name mismatch.";
                    if(flags.testFlag(LooseVerify) &&
                            _info.size() < _core->_infoKeys.size())
                        throw "Number of information entries is less than known information keys.";
                    else if(flags.testFlag(StrictVerify) &&
                                _info.size() != _core->_infoKeys.size())
                        throw "Number of information entries does not match known information keys.";
                }
            }
        } else if(flags.testFlag(StrictVerify))
            throw "Missing information entry point.";
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

Module::WeakMap ModularCore::_knownModules;
