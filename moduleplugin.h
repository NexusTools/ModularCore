#ifndef MODULEPLUGIN_H
#define MODULEPLUGIN_H

#include <QObject>

#include "module.h"

class ModulePlugin : public QObject
{
    friend class Module;
public:
    inline explicit ModulePlugin() {}

    inline Module::Ref provider() const{return _provider;}

protected:
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
    Module::Ref _provider;
};

#endif // MODULEPLUGIN_H
