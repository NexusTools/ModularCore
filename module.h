#ifndef MODULE_H
#define MODULE_H

#include <QSharedPointer>
#include <QVariantMap>
#include <QStringList>
#include <QLibrary>
#include <QHash>

class ModularCore;
class ModulePlugin;

class Module
{
    typedef QWeakPointer<Module> Weak;
    typedef QSharedPointer<Module> Ref;

    typedef QList<Weak> WeakList;
    typedef QList<Ref> List;

    typedef QHash<QString, Weak> WeakMap;
    typedef QHash<QString, Ref> Map;

    typedef ModulePlugin* (*CreatePlugin)();

    friend class ModularCore;
protected:
    inline explicit Module(QString libraryFile) : _lib(libraryFile) {}

    void loadEntryPoints(bool exportSymbols =false);

    ModulePlugin* createPlugin(QString type) {return createPlugin(_types.indexOf(type));}
    ModulePlugin* createPlugin(int type =0);

private:
    QString _name;
    QString _type;

    List _deps;
    QVariantMap _meta;
    QStringList _types;
    ModularCore* _core;
    QLibrary _lib;
};

#endif // MODULE_H
