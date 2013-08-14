#ifndef MODULEPLUGIN_H
#define MODULEPLUGIN_H

#include <QObject>
#include "global.h"
#include "module.h"

class MODULARCORESHARED_EXPORT ModulePlugin : public QObject
{
    Q_OBJECT

    friend class Module;
public:
    inline explicit ModulePlugin() {_core=0;}

// Module Helpers
    inline QString moduleName() const{return _provider->name();}
    inline QString moduleType() const{return _provider->type();}
    inline ModularCore* moduleCore() const{return _provider.isNull() ? _core : _provider->core();}
    inline Module* module() const{return _provider.data();}

protected:
    inline explicit ModulePlugin(ModularCore* core) {_core=core;}

// Plugin Helpers
    template <class T>
    inline T* createCompatiblePlugin(QVariantList args =QVariantList(), Module::PluginResolveScope scope =Module::ResolveDependancies) {
        if(!_provider)
            throw "No provider registered for this plugin.";

        return _provider->createCompatiblePlugin<T>(args, scope);
    }

    template <class T>
    inline QList<T*> createCompatiblePlugins(QVariantList args =QVariantList(), Module::PluginResolveScope scope =Module::ResolveExtensions) {
        if(!_provider)
            throw "No provider registered for this plugin.";

        return _provider->createCompatiblePlugins<T>(args, scope);
    }

private:
    ModularCore* _core;
    Module::Ref _provider;
};

#endif // MODULEPLUGIN_H
