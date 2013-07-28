#-------------------------------------------------
#
# Project created by QtCreator 2013-07-27T18:22:24
#
#-------------------------------------------------

QT       -= gui

TARGET = ModularCore
TEMPLATE = lib

DEFINES += MODULARCORE_LIBRARY

include(version.pri)

SOURCES += \
    modularcore.cpp

HEADERS += modularcore.h \
    global.h \
    macros.h \
    module.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
