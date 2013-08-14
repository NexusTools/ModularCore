#ifndef MACROS_H
#define MACROS_H

#include "module-defines.h"
#include "module-types.h"
#include "__macros.h"

#define ModuleString(String, Type) ModuleEntry(Type##StringType, (const void*)String)
#define ModuleMetaObject(MetaObject) ModuleEntry(MetaObjectType, (const void*)MetaObject)

#define DeclareModuleString(String, Type) << ModuleString(String, Type)
#define DeclareInfoString(String) DeclareModuleString(String, Info)
#define DeclarePlugin(Plugin) << ModuleMetaObject(&Plugin::staticMetaObject)
#define DeclareObjectInstance(Instance) << ModuleEntry(ObjectInstanceType, (const void*)Instance)
#define DeclareData(Type, ID, Data) << ModuleEntry(DataEntryType, ModuleDataEntry<Type, ID>(Data))

#define DeclareModuleInternals() __ModuleVerificationEntry __ModuleQtVersionEntry __ModuleQtPackageTagEntry __ModuleQtPackageDateEntry
#define DeclareModuleInfoStrings() DeclareInfoString(MODULE_LIB_NAME) DeclareInfoString(VERSION) \
    DeclareInfoString(AUTHORS) DeclareInfoString("Unknown") DeclareInfoString("Unknown")

#define BeginModuleNamespace(Class, Type) \
    extern "C" Q_DECL_EXPORT const ModuleEntryList ModuleEntryPoint_##Type##_##Class##_EntryList() \
        {static ModuleEntryList entries(ModuleEntryList() DeclareModuleInternals() DeclareModuleInfoStrings()

#define BeginModule(Class, Type) BeginModuleNamespace(Class, Type) DeclarePlugin(Class)
#define FinishModule() ); return entries;}

#endif // MACROS_H
