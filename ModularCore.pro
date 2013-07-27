#-------------------------------------------------
#
# Project created by QtCreator 2013-07-27T18:22:24
#
#-------------------------------------------------

QT       -= gui

TARGET = ModularCore
TEMPLATE = lib

DEFINES += MODULARCORE_LIBRARY

SOURCES += modularcore.cpp

HEADERS += modularcore.h\
        modularcore_global.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
