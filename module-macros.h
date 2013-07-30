#ifndef MACROS_H
#define MACROS_H

#include <project-version.h>
#include "module-types.h"

#ifndef MODULE_LIB_NAME
#warning "MODULE_LIB_NAME is not defined, using GenericLibrary"
#define MODULE_LIB_NAME "GenericLibrary"
#endif

#define MODULAR_CORE protected: \
        inline QString libraryName() const{return MODULE_LIB_NAME;} \
    \
    private:


#define ModuleString(String) ModuleEntry(StringType, (const void*)String)
#define ModuleMetaObject(MetaObject) ModuleEntry(MetaObjectType, (const void*)MetaObject)

#define DeclareString(String) << ModuleString(String)
#define DeclarePlugin(Plugin) << ModuleMetaObject(&Plugin::staticMetaObject)

#define BeginModuleNamespace(Class, Type) \
    extern "C" Q_DECL_EXPORT const ModuleEntryList ModuleEntryPoint_##Type##_##Class##_EntryList() {static ModuleEntryList entries(ModuleEntryList() DeclareString(MODULE_LIB_NAME) DeclareString(VERSION) DeclareString(AUTHORS) DeclareString("Unknown") DeclareString("Unknown")

#define BeginModule(Class, Type) BeginModuleNamespace(Class, Type) DeclarePlugin(Class)
#define FinishModule() ); return entries;}

#endif // MACROS_H
