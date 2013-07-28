#ifndef MODULE_H
#define MODULE_H

#include <QSharedPointer>
#include <QVariantMap>
#include <QStringList>
#include <QLibrary>
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
    enum LoadFlag {
        NoVerify = 0x0,
        LooseVerify = 0x1,
        StrictVerify = 0x2,
        ExportSymbols = 0x4
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
    inline QString version() const{return _version;}
    inline QStringList authors() const{return _authorList;}
    inline QString authorsString() const{return _authors;}
    inline bool loaded() const{return _lib.isLoaded();}

    inline QString libraryFile() const{return _lib.fileName();}
    inline QVariantMap metaData() const{return _meta;}

    void unload();
    void load(LoadFlags flags = LooseVerify);
    inline void reload(LoadFlags flags = LooseVerify) {unload();load(flags);}

    // Plugins
    inline QStringList plugins() const{return _plugins.keys();}

    ModulePlugin* createPlugin(QString type, QVariantList args =QVariantList());
    inline ModulePlugin* createPlugin(int type =0, QVariantList args =QVariantList()) {if(type < 0 || type >= _plugins.size()) throw "Invalid index, no such plugin"; return createPlugin(_plugins.keys().at(type), args);}

    //template <class T>
    //T* newInstance(QVariantList args =QVariantList()) {return (T*)createPlugin(TypeName(T), args);}


    const QMetaObject* pluginMeta(QString type) {if(!_plugins.contains(type)) throw QString("No plugin registered is named `%1`").arg(type); return _plugins.value(type);}
    inline const QMetaObject* pluginMeta(int type =0) {if(type < 0 || type >= _plugins.size()) throw "Invalid index, no such plugin"; return pluginMeta(_plugins.keys().at(type));}

protected:
    void loadDep(QString name, QString type);
    void loadEntryPoints(LoadFlags flags = LooseVerify);

private:
    inline explicit Module(QString name, QString type, QString libraryFile, ModularCore* core) :
        _name(name), _type(type), _entryBaseName(QString("ModuleEntryPoint_%1_%2_").arg(type, name)),
        _core(core), _lib(libraryFile) {}

    const QString _name;
    const QString _type;
    const QString _entryBaseName;

    QString _appName;
    QString _authors;
    QString _version;
    QStringList _info;
    QStringList _authorList;

    List _deps;
    Weak _self;
    QVariantMap _meta;
    ModularCore* _core;
    PluginMap _plugins;
    QLibrary _lib;
};

#endif // MODULE_H
