#include "modularcore.h"
#include "moduleplugin.h"
#include "module-defines.h"
#include "module.h"

#include <QCoreApplication>
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

void ModularCore::registerType(QString type, QString path, QString depNode) {
    if(_types.contains(type))
        throw QString("Type `%1` already registered.").arg(type);

    QString searchPath;
    const char* prefix = searchPathPrefix();
    if(prefix)
        searchPath += prefix + '-';
    searchPath += type;

    path = QDir::toNativeSeparators(path);

    QStringList paths;
    QString pwd = QDir::toNativeSeparators(QDir::currentPath());
    if(!pwd.endsWith(QDir::separator()))
        pwd += QDir::separator();
    paths << pwd;
    pwd += path;
    if(!pwd.endsWith(QDir::separator()))
        pwd += QDir::separator();
    paths << pwd;

    pwd = QDir::toNativeSeparators(QDir("..").absolutePath());
    if(!pwd.endsWith(QDir::separator()))
        pwd += QDir::separator();
    paths << pwd;
    pwd += path;
    if(!pwd.endsWith(QDir::separator()))
        pwd += QDir::separator();
    paths << pwd;

    foreach(QString appName, searchAppNames()) {
#ifdef Q_OS_UNIX
    pwd = QString("/usr/lib/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    pwd = QString("/opt/lib/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    pwd = QString("/lib/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    pwd = QString("/opt/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    appName = appName.toLower();

    pwd = QString("/usr/lib/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    pwd = QString("/opt/lib/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    pwd = QString("/lib/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';

    pwd = QString("/opt/%1/").arg(appName);
    paths << pwd;
    paths << pwd + path + '/';
#endif
}

    QDir::setSearchPaths(searchPath, paths);
    _types.insert(type, TypeInfo(path, depNode));
}

Module::Ref ModularCore::loadModule(QString libPath, NameTypePair nameType, TypeInfo typeInfo) {
    Module::Ref module = _knownModules.value(libPath).toStrongRef();
    if(!module) {
        qDebug() << "Parsing" << libPath;

        QFile library(libPath);
        if(library.open(QFile::ReadOnly)) {
            QVariantMap metaData;
            Module::List deps;

            int pos;
            QByteArray buffer;
            qDebug() << "Reading module" << libPath;
            while(!library.atEnd()) {
                if(buffer.length() >= 200)
                    buffer = buffer.mid(100);
                buffer += library.read(100);
                if((pos = buffer.indexOf(QString("<%1>").arg(libraryName()))) > -1) {
                    qDebug() << "Found xml definition" << libraryName();

                    buffer = buffer.mid(pos);
                    QString endNode = QString("</%1>").arg(libraryName());
                    forever {
                        if(library.atEnd())
                            throw "Unexpected End of File";

                        buffer += library.read(512);
                        pos = buffer.indexOf(endNode);
                        if(pos > 0) {
                            buffer = buffer.mid(0, pos + endNode.length());
                            break;
                        }
                    }
                    qDebug() << buffer;

                    int errCol;
                    int errLine;
                    QString errMsg;
                    QDomDocument dom;
                    if(dom.setContent(buffer, &errMsg, &errLine, &errCol)) {
                        QDomNodeList children = dom.childNodes();
#ifdef LEGACY_QT
                        for(uint i=0; i<children.length(); i++){
#else
                        for(int i=0; i<children.length(); i++){
#endif
                            QDomNode node = children.at(i);
                            if(node.nodeName() == libraryName()) {
                                metaData = nodeToVariant(node).toMap();
                                qDebug() << metaData;

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
                    } else
                        throw QString("Error parsing definition: %1 at %2:%3").arg(errMsg).arg(errLine).arg(errCol);
                    break;
                }
            }
            library.close();

            if(nameType.first.isEmpty()) {
                nameType.first = metaData.value("Name").toString();
                if(nameType.first.isEmpty())
                    throw "No name specified";
            }
            if(nameType.second.isEmpty()) {
                nameType.second = metaData.value("Type").toString();
                if(nameType.second.isEmpty())
                    throw "No type specified";
            }
            module = Module::create(nameType.first, nameType.second, deps, libPath, this);
            moduleVerify(module);
        }
    }

    return module;
}

#ifdef Q_OS_UNIX
#define LIB_SUFFIX "so"
#else
#define LIB_SUFFIX "dll"
#endif

Module::Ref ModularCore::loadModule(QString name, QString type) {
    TypeInfo typeInfo = _types.value(type);
    if(typeInfo.first.isEmpty())
        throw QString("No type named `%1` registered.").arg(type);

    Module::Ref module = _modules.value(type).value(name).toStrongRef();
    if(!module) {
        QString searchPath;
        const char* prefix = searchPathPrefix();
        if(prefix)
            searchPath += prefix + '-';
        searchPath += type;

        qDebug() << "Searching for module" << name << type;

        QStringList nameSearch;
        nameSearch << name;
#ifdef Q_OS_UNIX
        nameSearch << name.toLower();
        nameSearch << QString("lib%1").arg(name);
        nameSearch << QString("lib%1").arg(name.toLower());
#else
        for(int i=0; i<=255; i++)
            nameSearch << QString("%1%2").arg(name).arg(i);
#endif
        QString dir = name + QDir::separator();
        nameSearch << dir + name;
#ifdef Q_OS_UNIX
        nameSearch << dir + name.toLower();
        nameSearch << dir + QString("lib%1").arg(name);
        nameSearch << dir + QString("lib%1").arg(name.toLower());
#else
        for(int i=0; i<=255; i++)
            nameSearch << dir + QString("%1%2").arg(name).arg(i);
#endif

        QString buildPath = name + QDir::separator();
#ifdef Q_OS_WIN
#ifdef DEBUG_MODE
        buildPath += "debug\\";
#else
        buildPath += "release\\";
#endif
#endif
        QFileInfo buildDir(QString("%1:%2").arg(searchPath).arg(buildPath));
        qDebug() << "Searching for" << buildPath << "in" << QDir::searchPaths(searchPath);
        QFileInfo moduleFile;
        if(buildDir.isDir()) {
            QString path = QDir::toNativeSeparators(buildDir.absolutePath());
            if(!path.endsWith(QDir::separator()))
                path += QDir::separator();
            qDebug() << "Build directory found" << path;
            qDebug() << "Searching for" << nameSearch;
            foreach(QString libName, nameSearch) {
                moduleFile.setFile(QString("%1%2." LIB_SUFFIX).arg(path).arg(libName));
                if(!moduleFile.exists())
                    continue;

                qDebug() << "Found module" << moduleFile.absoluteFilePath();
                break;
            }
        }

        if(!moduleFile.exists())
            foreach(QString libName, nameSearch) {
                moduleFile.setFile(QString("%1:%2." LIB_SUFFIX).arg(searchPath).arg(libName));
                if(!moduleFile.exists())
                    continue;

                qDebug() << "Found module" << moduleFile.absoluteFilePath();
                break;
            }

        if(!moduleFile.exists())
            throw "Cannot locate module";

        qDebug() << "Loading" << type << name;
        module = loadModule(moduleFile.absoluteFilePath(), NameTypePair(name, type), typeInfo);
        if(module.isNull())
            throw QString("Cannot read requested module `%1` of type `%2`.").arg(name).arg(type);

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
    if((flags.testFlag(LooseVerify) && !_info.isEmpty()) |
                            flags.testFlag(StrictVerify)) {
        if(_info.isEmpty())
            throw "No information strings provided but are required for strict verification.";

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
            case GenericVerifyStringType:
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

            case QtPackageTagStringType:
            case QtPackageDateStringType:
                continue; // Not used

            case ObjectInstanceType:
            {
                QObject* obj = (QObject*)entry.second;
                Q_ASSERT(obj);
                _instances << obj;
                continue;
            }

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
    if(!_data.isEmpty())
        qDebug() << "Data detected" << _data;
    if(!_plugins.isEmpty())
        qDebug() << "Plugins detected" << _plugins.keys();
    if(!_instances.isEmpty())
        qDebug() << "Instances detected" << _instances;
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

bool Module::checkSubclass(const QMetaObject* pluginMeta, const QMetaObject* desiredMeta) {
    const QMetaObject* superMeta = pluginMeta;
    while(superMeta != desiredMeta) {
        superMeta = superMeta->superClass();
        if(!superMeta)
            break;
    }

    return superMeta;
}

bool Module::isModulePlugin(const QMetaObject * pluginMeta) {
    return checkSubclass(pluginMeta, &ModulePlugin::staticMetaObject);
}

ModulePlugin* Module::createPluginInstance(const QMetaObject* metaObject, QVariantList args) {
    QObject* plugin = createInstance(metaObject, args);
    Q_ASSERT(ModulePlugin::staticMetaObject.cast(plugin));
    ((ModulePlugin*) plugin)->_provider = pointer();
    return (ModulePlugin*) plugin;
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
    return metaObject->newInstance(val[0], val[1], val[2], val[3], val[4],
                                    val[5], val[6], val[7], val[8], val[9]);
}

Module::WeakMap ModularCore::_knownModules;
