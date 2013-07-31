#ifndef MACROS_H
#define MACROS_H

#include <project-version.h>
#include "module-types.h"

#ifndef MODULE_LIB_NAME
#warning "MODULE_LIB_NAME is not defined, using 'GenericLibrary'"
#define MODULE_LIB_NAME "GenericLibrary"
#endif

#ifdef MODULE_VERIFY_STRING
#define __MODULE_VERIFICATION_STRING inline const char* verificationString() const{return MODULE_VERIFY_STRING;}
#else
#define __MODULE_VERIFICATION_STRING /* No Verification String */
#endif

#define MODULAR_CORE protected: \
        inline QString libraryName() const{return MODULE_LIB_NAME;} \
        __MODULE_VERIFICATION_STRING \
    \
    private:


#define ModuleString(String, Type) ModuleEntry(Type##StringType, (const void*)String)
#define ModuleMetaObject(MetaObject) ModuleEntry(MetaObjectType, (const void*)MetaObject)

#define DeclareModuleString(String, Type) << ModuleString(String, Type)
#define DeclareInfoString(String) DeclareModuleString(String, Info)
#define DeclarePlugin(Plugin) << ModuleMetaObject(&Plugin::staticMetaObject)
#define DeclareObjectInstance(Instance) << ModuleEntry(ObjectInstanceType, (const void*)Instance)
#define DeclareData(Type, ID, Data) << ModuleEntry(DataEntryType, ModuleDataEntry<Type, ID>(Data))

#ifdef MODULE_VERIFY_STRING
#define __ModuleVerificationEntry DeclareModuleString(MODULE_VERIFY_STRING, Verify)
#else
#define __ModuleVerificationEntry /* No Verification String */
#endif
#define __ModuleQtVersionEntry << ModuleEntry(QtVersionType, (const void*)QT_VERSION)

#define BeginModuleNamespace(Class, Type) \
    extern "C" Q_DECL_EXPORT const ModuleEntryList ModuleEntryPoint_##Type##_##Class##_EntryList() \
{static ModuleEntryList entries(ModuleEntryList() __ModuleVerificationEntry __ModuleQtVersionEntry DeclareInfoString(MODULE_LIB_NAME) \
            DeclareInfoString(VERSION) DeclareInfoString(AUTHORS) DeclareInfoString("Unknown") DeclareInfoString("Unknown")

#define BeginModule(Class, Type) BeginModuleNamespace(Class, Type) DeclarePlugin(Class)
#define FinishModule() ); return entries;}

#endif // MACROS_H
