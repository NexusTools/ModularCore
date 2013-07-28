#ifndef MODULARCORE_H
#define MODULARCORE_H

#include <QVariantMap>

#include "module.h"
#include "global.h"

class MODULARCORESHARED_EXPORT ModularCore
{
    typedef QHash<QString, Module::WeakMap> LoadedModules;
    typedef QHash<QString, QString> TypePaths;

    friend class Module;
public:
    Module::List modulesByType(QString type) {
        Module::List modules;
        foreach(Module::Weak mod, _modules.value(type).values()) {
            if(!mod.isNull())
                modules << mod.toStrongRef();
        }

        return modules;
    }

    Module::Ref module(QString name, QString type) {
        return _modules.value(type).value(name).toStrongRef();
    }

protected:
    explicit ModularCore() : _infoKeys(QStringList() << "AppName" << "Version" << "Authors" << "Website" << "Source") {}

    // Module Controls
    Module::Ref loadModule(QString name, QString type);
    void unloadModule(QString name, QString type);

    inline void registerType(QString type, QString path) {_types.insert(type, path);}

    // Module Events
    virtual void moduleVerify(Module::Ref) {}
    virtual void moduleLoaded(Module::Ref) {}
    virtual void moduleUnloaded(Module::Ref) {}
    virtual Module::List moduleMetaData(QVariantMap) {return Module::List();}

    // The name of the modules this core works with
    virtual QString libraryName() const{return "GenericModule";}

private:
    QString _name;
    TypePaths _types;
    QStringList _infoKeys;
    LoadedModules _modules;
    static Module::WeakMap _knownModules;

};

#endif // MODULARCORE_H
