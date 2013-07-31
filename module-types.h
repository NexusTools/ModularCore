#ifndef MODULETYPES_H
#define MODULETYPES_H

#include <QVariant>
#include <QVector>
#include <QPair>

enum ModuleEntryType {
    InfoStringType = 0x0,
    MetaObjectType = 0x1,
    GenericVerifyStringType = 0x2,

    VerifyStringType = 0x5,
    FunctionPointerType = 0x6,
    ObjectInstanceType = 0x7,
    DataEntryType = 0x8,

    QtPackageTagStringType = 0xf0,
    QtPackageDateStringType = 0xf1,
    QtVersionType = 0xff
};

typedef QPair<ModuleEntryType, const void*> ModuleEntry;
typedef QVector<ModuleEntry> ModuleEntryList;
typedef QPair<quint64, QVariant> ModuleData;

template <class T, quint64 id>
inline const void* ModuleDataEntry(T def =T()) {
    static ModuleData _data(id, def);
    return &_data;
}

#endif // MODULETYPES_H
