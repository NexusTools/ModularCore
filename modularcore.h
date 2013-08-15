#ifndef MODULARCORE_H
#define MODULARCORE_H

#include <QVariantMap>

#include "module.h"
#include "global.h"

class MODULARCORESHARED_EXPORT ModularCore
{
    friend class Module;
public:
    typedef QHash<QString, Module::WeakMap> LoadedModules;
    typedef QPair<QString, QString> NameTypePair;
    typedef QPair<QString, QString> TypeInfo;
    typedef QHash<QString, TypeInfo> Types;

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

    static const Module::Version coreVersion();

protected:
    inline explicit ModularCore() : _infoKeys(QStringList() << "AppName" << "Version" << "Authors" << "Website" << "Source" << "Type" << "Namespace") {}

    // Module Controls
    Module::Ref loadModule(QString file, NameTypePair nameType, TypeInfo typeInfo =TypeInfo());
    Module::Ref loadModule(QString name, QString type);
    Module::Ref loadModuleByDefinition(QVariantMap def);
    void unloadModule(QString name, QString type);

    void registerType(QString type, QString path, QString depNode =QString());

    // Module Events
    virtual void moduleVerify(const Module::Ref) {}
    virtual void moduleLoaded(const Module::Ref) {}
    virtual void moduleUnloaded(const Module::Ref) {}
    virtual void moduleInformation(const Module::Ref) {}
    virtual void moduleEntries(const Module::Ref, const ModuleEntryList &);

    virtual Module::List moduleMetaData(QVariantMap) {return Module::List();}

    // The name of the modules this core works with
    virtual QString libraryName() const{return "GenericModule";}
    virtual const char* verificationString() const{return 0;}
    virtual const char* searchPathPrefix() const{return 0;}
    virtual QStringList searchAppNames() const{return QStringList() << "ModularCore";}

private:
    Types _types;
    QString _name;
    QStringList _infoKeys;
    LoadedModules _modules;
    static Module::WeakMap _knownModules;

};

#endif // MODULARCORE_H
