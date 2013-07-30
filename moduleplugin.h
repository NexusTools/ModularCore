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

private:
    Module::Ref _provider;
};

#endif // MODULEPLUGIN_H
