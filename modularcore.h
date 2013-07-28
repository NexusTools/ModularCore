#ifndef MODULARCORE_H
#define MODULARCORE_H

#include <QVariantMap>

#include "global.h"
#include "module.h"

class MODULARCORESHARED_EXPORT ModularCore
{
    typedef QHash<QString, Module::WeakMap> KnownModules;
    typedef QHash<QString, QString> TypePaths;
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
    explicit ModularCore() {}

    // Module Controls
    Module::Ref loadModule(QString name, QString type);
    void unloadModule(QString name, QString type);

    inline void registerType(QString type, QString path) {
        _types.insert(type, path);
    }

    // Module Events
    virtual void moduleMetaData(Module::Ref, QVariantMap) {}

    virtual void moduleStarted(Module::Ref) {}
    virtual void moduleStopped(Module::Ref) {}

    // The name of the modules this core works with
    virtual QString moduleName() const{return "GenericModule";}

private:
    QString _name;
    KnownModules _modules;
    TypePaths _types;
};

#endif // MODULARCORE_H
