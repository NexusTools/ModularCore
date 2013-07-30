#ifndef MODULETYPES_H
#define MODULETYPES_H

#include <QList>
#include <QPair>

enum ModuleEntryType {
    StringType,
    MetaObjectType
};

typedef QPair<ModuleEntryType, const void*> ModuleEntry;
typedef QList<ModuleEntry> ModuleEntryList;

#endif // MODULETYPES_H
