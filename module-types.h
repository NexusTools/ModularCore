#ifndef MODULETYPES_H
#define MODULETYPES_H

#include <QVariant>
#include <QList>
#include <QPair>

enum ModuleEntryType {
    InfoStringType = 0x0,
    MetaObjectType = 0x1,

    VerifyStringType = 0x5,
    FunctionPointerType = 0x6,
    DataEntryType = 0x8
};

typedef QPair<ModuleEntryType, const void*> ModuleEntry;
typedef QPair<quint64, QVariant> ModuleData;
typedef QList<ModuleEntry> ModuleEntryList;

template <class T, quint64 id>
inline const void* ModuleDataEntry(T def =T()) {
    static ModuleData _data(id, def);
    return &_data;
}

#endif // MODULETYPES_H
