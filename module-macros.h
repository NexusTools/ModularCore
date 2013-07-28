#ifndef MACROS_H
#define MACROS_H

#include <project-version.h>

#include <module.h>

#ifndef MODULE_LIB_NAME
#warning "MODULE_LIB_NAME is not defined, using GenericLibrary"
#define MODULE_LIB_NAME "GenericLibrary"
#endif

#define MODULAR_CORE protected: \
        inline QString libraryName() const{return MODULE_LIB_NAME;} \
    \
    private:

#define BeginModule(Class, Type) \
    extern "C" Q_DECL_EXPORT const QStringList ModuleEntryPoint_##Type##_##Class##_Information() {static QStringList info(QStringList() << MODULE_LIB_NAME << VERSION << AUTHORS << "Unknown" << "Unknown"); return info;} \
    extern "C" Q_DECL_EXPORT const ConstructorList ModuleEntryPoint_##Type##_##Class##_Constructors() {static ConstructorList classes(ConstructorList() << &Class::staticMetaObject

#define DeclarePlugin(Plugin) << &Plugin::staticMetaObject
#define EndModule() ); return classes;}

#endif // MACROS_H
