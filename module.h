#ifndef MODULE_H
#define MODULE_H

#include <QSharedPointer>
#include <QVariantMap>
#include <QStringList>
#include <QLibrary>
#include <QDebug>
#include <QHash>

#include "global.h"
#include "module-types.h"

class ModularCore;
class ModulePlugin;

typedef QList<const QMetaObject*> ConstructorList;

class MODULARCORESHARED_EXPORT Module
{
    struct Version {
        quint8 major;
        quint8 minor;
        quint32 revision;

        QString branch;
        QString source;
    };

    typedef QMap<quint64, QVariant> DataIndexList;
    typedef QMap<QVariant::Type, DataIndexList> DataMap;

    typedef QHash<QString, const QMetaObject*> PluginMap;
    typedef QList<QObject*> InstanceList;

    // Entry Points
    typedef ConstructorList (*Constructors)();
    typedef QStringList (*Information)();

    // Preferred Method
    typedef ModuleEntryList (*EntryList)();


    friend class ModularCore;
public:
    struct Author {
        QString name;
        QString email;
        QStringList altNames;
    };

    enum PluginResolveFlag {
        ResolveSelf = 0x1,
        ResolveDependancies = 0x2,
        ResolveExtensions = 0x4
    };
    Q_DECLARE_FLAGS(PluginResolveScope, PluginResolveFlag)

    typedef QSharedPointer<Author> AuthorRef;
    typedef QList<AuthorRef> AuthorList;

    enum LoadFlag {
        NoVerify = 0x0,
        LooseVerify = 0x1,
        StrictVerify = 0x2,

        ExportSymbols = 0x4,
        IgnoreLibraryName = 0x8,
        LazyLoadSymbols = 0x10,
        AllowGenericLibs = 0x20,

        Library = (LooseVerify | ExportSymbols | AllowGenericLibs)
    };
    Q_DECLARE_FLAGS(LoadFlags, LoadFlag)

    typedef QWeakPointer<Module> Weak;
    typedef QSharedPointer<Module> Ref;

    typedef QHash<QString, Weak> WeakMap;
    typedef QHash<QString, Ref> Map;

    typedef QList<Weak> WeakList;
    typedef QList<Ref> List;

    virtual ~Module() {qDebug() << "Module unloaded" << _type << _name;}

    inline QString name() const{return _name;}
    inline QString type() const{return _type;}
    inline QString website() const{return _info.isEmpty() ? "Unknown" : _info.at(3);}
    inline QString source() const{return _version.source;}
    inline QString sourceBranch() const{return _version.branch;}

    inline QString version() const{return _info.isEmpty() ? "Unknown" : _info.at(1);}
    inline quint8 verMajor() const{return _version.major;}
    inline quint8 verMinor() const{return _version.minor;}
    inline quint32 verPat() const{return _version.revision;}

    inline AuthorList authors() const{return _authorList;}
    inline QString authorsString() const{return _info.isEmpty() ? "Unknown" : _info.at(2);}

    inline ModularCore* core() const{return _core;}
    inline QVariantMap metaData() const{return _meta;}
    inline QString libraryFile() const{return _lib.fileName();}

    inline Ref pointer() const{return _self.toStrongRef();}

    void unload();
    void load(LoadFlags flags = LooseVerify);
    inline bool loaded() const{return _lib.isLoaded();}
    inline void reload(LoadFlags flags = LooseVerify) {unload();load(flags);}

    // Plugins
    inline QStringList plugins() const{return _plugins.keys();}

    inline ModulePlugin* createPlugin(QString type, QVariantList args =QVariantList()) {return createPluginInstance(plugin(type), args);}
    inline ModulePlugin* createPlugin(int type =0, QVariantList args =QVariantList()) {if(type < 0 || type >= _plugins.size()) throw "Invalid index, no such plugin"; return createPlugin(_plugins.keys().at(type), args);}

    inline const QMetaObject* plugin(int type =0) {if(type < 0 || type >= _plugins.size()) throw "Invalid index, no such plugin"; return plugin(_plugins.keys().at(type));}
    inline const QMetaObject* plugin(QString type) {if(!_plugins.contains(type)) throw QString("No plugin registered named `%1`").arg(type); return _plugins.value(type);}

    template <class T>
    bool isPluginCompatible(const QMetaObject* pluginMeta) {
        return checkSubclass(pluginMeta, &T::staticMetaObject);
    }

    template <class T>
    bool isPluginCompatible(QString type) {
        return isPluginCompatible<T>(plugin(type));
    }

    template <class T>
    const QMetaObject* findCompatiblePlugin(PluginResolveScope scope =ResolveSelf) {
        qDebug() << "Searching for plugin compatible with" << T::staticMetaObject.className();

        if(scope.testFlag(ResolveSelf))
            foreach(const QMetaObject* pluginMeta, _plugins.values()) {
                if(isPluginCompatible<T>(pluginMeta))
                    return pluginMeta;
            }

        if(scope.testFlag(ResolveDependancies))
            foreach(Module::Ref dep, _deps)
                foreach(const QMetaObject* pluginMeta, dep->_plugins.values())
                    if(isPluginCompatible<T>(pluginMeta))
                        return pluginMeta;

        if(scope.testFlag(ResolveExtensions))
            foreach(Module::Ref ext, _extensions)
                foreach(const QMetaObject* pluginMeta, ext->_plugins.values())
                    if(isPluginCompatible<T>(pluginMeta))
                        return pluginMeta;

        return 0;
    }

    template <class T>
    QList<const QMetaObject*> findCompatiblePlugins(PluginResolveScope scope =ResolveSelf) {
        qDebug() << "Searching for plugins compatible with" << T::staticMetaObject.className();
        QList<const QMetaObject*> _foundPlugins;

        if(scope.testFlag(ResolveSelf))
            foreach(const QMetaObject* pluginMeta, _plugins.values())
                if(isPluginCompatible<T>(pluginMeta))
                    _foundPlugins << pluginMeta;

        if(scope.testFlag(ResolveDependancies))
            foreach(Module::Ref dep, _deps)
                foreach(const QMetaObject* pluginMeta, dep->_plugins.values())
                    if(isPluginCompatible<T>(pluginMeta))
                        _foundPlugins << pluginMeta;

        if(scope.testFlag(ResolveExtensions))
            foreach(Module::Ref ext, _extensions)
                foreach(const QMetaObject* pluginMeta, ext->_plugins.values())
                    if(isPluginCompatible<T>(pluginMeta))
                        _foundPlugins << pluginMeta;

        return _foundPlugins;
    }

    template <class T>
    T* createCompatiblePlugin(QVariantList args =QVariantList(), PluginResolveScope scope =ResolveSelf) {
        const QMetaObject* pluginMetaObject = findCompatiblePlugin<T>(scope);
        bool pluginType = isModulePlugin(&T::staticMetaObject);

        if(pluginMetaObject) {
            QObject* instance;
            if(pluginType)
                instance = (QObject*)createPluginInstance(pluginMetaObject, args);
            else
                instance = createInstance(pluginMetaObject, args);
            if(instance) {
                Q_ASSERT(T::staticMetaObject.cast(instance));
                return (T*)instance;
            }
        }
        return 0;
    }

    template <class T>
    QList<T*> createCompatiblePlugins(QVariantList args =QVariantList(), PluginResolveScope scope =ResolveSelf) {
        QList<T*> objects;
        foreach(const QMetaObject* pluginMetaObject, findCompatiblePlugins<T>(scope))
            if(pluginMetaObject) {
                QObject* instance = createInstance(pluginMetaObject, args);
                if(instance) {
                    Q_ASSERT(T::staticMetaObject.cast(instance));
                    objects << (T*)instance;
                }
            }

        return objects;
    }

    template <class T>
    T* instance(PluginResolveScope scope =ResolveSelf) {
        if(scope.testFlag(ResolveSelf))
            foreach(QObject* obj, _instances) {
                T* i = qobject_cast<T*>(obj);
                if(i)
                    return i;
            }

        if(scope.testFlag(ResolveDependancies))
            foreach(Module::Ref dep, _deps)
                foreach(QObject* obj, dep->_instances) {
                    T* i = qobject_cast<T*>(obj);
                    if(i)
                        return i;
                }

        if(scope.testFlag(ResolveExtensions))
            foreach(Module::Ref ext, _extensions)
                foreach(QObject* obj, ext->_instances) {
                    T* i = qobject_cast<T*>(obj);
                    if(i)
                        return i;
                }


        return 0;
    }

    template <class T>
    QList<T*> instances(PluginResolveScope scope =ResolveSelf) {
        QList<T*> instances;

        if(scope.testFlag(ResolveSelf))
            foreach(QObject* obj, _instances) {
                T* i = qobject_cast<T*>(obj);
                if(i)
                    instances << i;
            }

        if(scope.testFlag(ResolveDependancies))
            foreach(Module::Ref dep, _deps)
                foreach(QObject* obj, dep->_instances) {
                    T* i = qobject_cast<T*>(obj);
                    if(i)
                        instances << i;
                }

        if(scope.testFlag(ResolveExtensions))
            foreach(Module::Ref ext, _extensions)
                foreach(QObject* obj, ext->_instances) {
                    T* i = qobject_cast<T*>(obj);
                    if(i)
                        instances << i;
                }


        return instances;
    }

protected:
    void loadDep(QString name, QString type);
    void loadEntryPoints(LoadFlags flags = LooseVerify);
    void processEntries(const ModuleEntryList&);
    void processInfoStrings(LoadFlags flags);

    bool checkSubclass(const QMetaObject*, const QMetaObject*);
    bool isModulePlugin(const QMetaObject*);
    QObject *createInstance(const QMetaObject* metaObject, QVariantList args =QVariantList());
    ModulePlugin *createPluginInstance(const QMetaObject* metaObject, QVariantList args =QVariantList());

private:
    inline explicit Module(QString name, QString type, List deps, QString libraryFile, ModularCore* core) :
        _name(name), _type(type), _entryBaseName(QString("ModuleEntryPoint_%1_%2_").arg(type, name)),
        _strong(this), _deps(deps), _self(_strong.toWeakRef()), _lib(libraryFile), _core(core) {_version.major=0;_version.minor=0;_version.revision=0;}

    inline static Module::Ref create(QString name, QString type, List deps, QString libraryFile, ModularCore* core) {
        Module::Ref ref = (new Module(name, type, deps, libraryFile, core))->pointer();
        ref->_strong.clear();
        return ref;
    }

    inline static Module::Ref create(QString name, QString type, QString libraryFile, ModularCore* core) {
        return create(name, type, List(), libraryFile, core);
    }

    const QString _name;
    const QString _type;
    const QString _entryBaseName;

    Ref _strong;
    QStringList _info;
    AuthorList _authorList;
    LoadFlags _loadFlags;
    Version _version;

    const List _deps;
    const Weak _self;

    QLibrary _lib;
    QVariantMap _meta;
    PluginMap _plugins;
    InstanceList _instances;
    ModularCore* _core;
    List _extensions;
    DataMap _data;
};

#endif // MODULE_H
