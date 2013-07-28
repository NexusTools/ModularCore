#ifndef MODULEPLUGIN_H
#define MODULEPLUGIN_H

#include <QObject>

#include "module.h"

class ModulePlugin : public QObject
{
public:
    inline explicit ModulePlugin() {}

    inline QString type() const{return _type;}
    inline Module::Ref provider() const{return _provider;}

private:
    QString _type;
    Module::Ref _provider;
};

#endif // MODULEPLUGIN_H
