#ifndef MODULARCORE_H
#define MODULARCORE_H

#include <QVariantMap>

#include "module.h"
#include "global.h"

class MODULARCORESHARED_EXPORT ModularCore
{
    typedef QHash<QString, Module::WeakMap> LoadedModules;
    typedef QPair<QString, QString> TypeInfo;
    typedef QHash<QString, TypeInfo> Types;

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

    Module::Ref module(QString name, QString type ="Module") {
        return _modules.value(type).value(name).toStrongRef();
    }

protected:
    explicit ModularCore() : _infoKeys(QStringList() << "AppName" << "Version" << "Authors" << "Website" << "Source") {}

    // Module Controls
    Module::Ref loadModule(QString name, QString type);
    Module::Ref loadModuleByDefinition(QVariantMap def);
    void unloadModule(QString name, QString type);

    inline void registerType(QString type, QString path, QString depNode =QString()) {_types.insert(type, TypeInfo(path, depNode));}

    // Module Events
    virtual void moduleVerify(const Module::Ref) {}
    virtual void moduleLoaded(const Module::Ref) {}
    virtual void moduleUnloaded(const Module::Ref) {}
    virtual void moduleInformation(const Module::Ref) {}
    virtual void moduleEntries(const Module::Ref, const ModuleEntryList &);

    virtual Module::List moduleMetaData(QVariantMap) {return Module::List();}

    // The name of the modules this core works with
    virtual QString libraryName() const{return "GenericModule";}

private:
    Types _types;
    QString _name;
    QStringList _infoKeys;
    LoadedModules _modules;
    static Module::WeakMap _knownModules;

};

#endif // MODULARCORE_H
