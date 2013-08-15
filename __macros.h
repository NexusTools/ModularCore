#ifndef __MACROS_H
#define __MACROS_H

// Internal helper macros

#ifdef MODULE_BUILD_GENERIC_LIB
#ifdef MODULE_LIB_NAME
#undef MODULE_LIB_NAME
#endif
#define MODULE_LIB_NAME __MODULE_GENERIC_LIB_NAME

#ifdef MODULE_VERIFY_STRING
#undef MODULE_VERIFY_STRING
#endif
#define MODULE_VERIFY_STRING __MODULE_GENERIC_VERIFICATION_STRING
#else
#ifndef MODULE_LIB_NAME
#define MODULE_LIB_NAME __MODULE_GENERIC_LIB_NAME
#endif

#include <QMetaObject>
#include <QStringList>

inline QStringList __app_Names_For_QMetaObject__(const QMetaObject* meta) {
    QStringList appNames;
    while(meta) {
        if(QString(meta->className()).startsWith('Q'))
            break; // Only check for non-Qt classes

        appNames << meta->className();
        meta = meta->superClass();
    }

    appNames << "ModularCore";
    return appNames;
}

#define MODULAR_CORE protected: \
        virtual QString libraryName() const{return MODULE_LIB_NAME;} \
        virtual QString searchPrefix() const{return staticMetaObject.className();} \
        virtual QStringList searchAppNames() const{ \
            static QStringList appNames(__app_Names_For_QMetaObject__(&staticMetaObject)); \
            return appNames; \
        } \
        __MODULE_VERIFICATION_STRING \
    \
    private:
#endif

#ifdef MODULE_VERIFY_STRING
#define __MODULE_VERIFICATION_STRING virtual const char* verificationString() const{return MODULE_VERIFY_STRING;}
#else
#define __MODULE_VERIFICATION_STRING /* No Verification String */
#endif


#if defined(MODULE_BUILD_GENERIC_LIB)
#define __ModuleVerificationEntry DeclareModuleString(MODULE_VERIFY_STRING, GenericVerify)
#elif defined(MODULE_VERIFY_STRING)
#define __ModuleVerificationEntry DeclareModuleString(MODULE_VERIFY_STRING, Verify)
#else
#define __ModuleVerificationEntry /* No Verification String */
#endif

#define __ModuleQtVersionEntry << ModuleEntry(QtVersionType, (const void*)QT_VERSION)

#if defined(QT_PACKAGE_TAG) && !defined(MODULE_BUILD_GENERIC_LIB)
#define __ModuleQtPackageTagEntry DeclareModuleString(QT_PACKAGE_TAG, QtPackageTag)
#else
#define __ModuleQtPackageTagEntry /* No package tag */
#endif

#if defined(QT_PACKAGEDATE_STR) && !defined(MODULE_BUILD_GENERIC_LIB)
#define __ModuleQtPackageDateEntry DeclareModuleString(QT_PACKAGEDATE_STR, QtPackageDate)
#else
#define __ModuleQtPackageDateEntry /* No package tag */
#endif

#endif // __MACROS_H
