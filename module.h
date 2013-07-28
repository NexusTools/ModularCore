#ifndef MODULE_H
#define MODULE_H

#include <QSharedPointer>
#include <QVariantMap>
#include <QStringList>
#include <QLibrary>
#include <QDebug>
#include <QHash>

class ModularCore;
class ModulePlugin;

typedef QList<const QMetaObject*> ConstructorList;

class Module
{
    typedef QHash<QString, const QMetaObject*> PluginMap;

    // Entry Points
    typedef ConstructorList (*Constructors)();
    typedef QStringList (*Information)();

    friend class ModularCore;
public:
    struct Author {
        QString name;
        QString email;
        QStringList altNames;
    };

    enum LoadFlag {
        NoVerify = 0x0,
        LooseVerify = 0x1,
        StrictVerify = 0x2,
        ExportSymbols = 0x4,
        IgnoreLibraryName = 0x8
    };
    Q_DECLARE_FLAGS(LoadFlags, LoadFlag)

    typedef QWeakPointer<Module> Weak;
    typedef QSharedPointer<Module> Ref;

    typedef QHash<QString, Weak> WeakMap;
    typedef QHash<QString, Ref> Map;

    typedef QList<Weak> WeakList;
    typedef QList<Ref> List;

    inline QString name() const{return _name;}
    inline QString type() const{return _type;}
    inline QString website() const{return _info.isEmpty() ? "Unknown" : _info.at(3);}
    inline QString source() const{return _info.isEmpty() ? "Unknown" : _info.at(4);}
    inline QString sourceBranch() const{return _branch;}

    inline QString version() const{return _info.isEmpty() ? "Unknown" : _info.at(1);}
    inline quint16 verMajor() const{return _versionParts[0];}
    inline quint16 verMinor() const{return _versionParts[1];}
    inline quint16 verPat() const{return _versionParts[2];}

    inline QVector<Author> authors() const{return _authorList;}
    inline QString authorsString() const{return _info.isEmpty() ? "Unknown" : _info.at(2);}

    inline QString libraryFile() const{return _lib.fileName();}
    inline QVariantMap metaData() const{return _meta;}

    void unload();
    void load(LoadFlags flags = LooseVerify);
    inline bool loaded() const{return _lib.isLoaded();}
    inline void reload(LoadFlags flags = LooseVerify) {unload();load(flags);}

    // Plugins
    inline QStringList plugins() const{return _plugins.keys();}

    ModulePlugin* createPlugin(QString type, QVariantList args =QVariantList());
    inline ModulePlugin* createPlugin(int type =0, QVariantList args =QVariantList()) {if(type < 0 || type >= _plugins.size()) throw "Invalid index, no such plugin"; return createPlugin(_plugins.keys().at(type), args);}

    inline const QMetaObject* plugin(int type =0) {if(type < 0 || type >= _plugins.size()) throw "Invalid index, no such plugin"; return plugin(_plugins.keys().at(type));}
    const QMetaObject* plugin(QString type) {if(!_plugins.contains(type)) throw QString("No plugin registered is named `%1`").arg(type); return _plugins.value(type);}

    template <class T>
    bool isPluginCompatible(const QMetaObject* pluginMeta) {
        const QMetaObject* superMeta = pluginMeta;
        qDebug() << "Checking compatibility" << pluginMeta->className();
        while(superMeta != &T::staticMetaObject) {
            superMeta = superMeta->superClass();
            if(!superMeta)
                break;

            qDebug() << "Checking super class" << superMeta->className();
        }

        return superMeta;
    }

    template <class T>
    bool isPluginCompatible(QString type) {
        return isPluginCompatible<T>(plugin(type));
    }

    template <class T>
    const QMetaObject* findCompatiblePlugin() {
        qDebug() << "Searching for plugin compatible with" << T::staticMetaObject.className();
        foreach(const QMetaObject* pluginMeta, _plugins.values()) {
            if(isPluginCompatible<T>(pluginMeta))
                return pluginMeta;
        }
        return 0;
    }

    template <class T>
    T* createCompatiblePlugin(QVariantList args =QVariantList()) {
        const QMetaObject* pluginMetaObject = findCompatiblePlugin<T>();
        if(pluginMetaObject) {
            QObject* instance = createInstance(pluginMetaObject, args);
            if(instance) {
                T* cInstance = (T*)T::staticMetaObject.cast(instance);
                if(!cInstance) {
                    delete instance;
                    throw QString("%1 cannot be cast to %2").arg(pluginMetaObject->className()).arg(T::staticMetaObject.className());
                }
                return cInstance;
            }
        }
        return 0;
    }

protected:
    void loadDep(QString name, QString type);
    void loadEntryPoints(LoadFlags flags = LooseVerify);

    QObject *createInstance(const QMetaObject* metaObject, QVariantList args =QVariantList());

private:
    inline explicit Module(QString name, QString type, QString libraryFile, ModularCore* core) :
        _name(name), _type(type), _entryBaseName(QString("ModuleEntryPoint_%1_%2_").arg(type, name)),
        _branch("unknown"), _core(core), _lib(libraryFile) {*_versionParts = 0;}

    const QString _name;
    const QString _type;
    const QString _entryBaseName;

    QStringList _info;
    QVector<Author> _authorList;
    quint16 _versionParts[3];
    QString _branch;

    List _deps;
    Weak _self;
    QVariantMap _meta;
    ModularCore* _core;
    PluginMap _plugins;
    QLibrary _lib;
};

#endif // MODULE_H
