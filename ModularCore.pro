#-------------------------------------------------
#
# Project created by QtCreator 2013-07-27T18:22:24
#
#-------------------------------------------------

QT       += xml
QT       -= gui

!greaterThan(QT_MAJOR_VERSION, 4) {
	DEFINES += LEGACY_QT
}

TARGET = ModularCore
TEMPLATE = lib

DEFINES += MODULARCORE_LIBRARY

include(version.pri)

CONFIG(debug, debug|release): DEFINES += IDE_MODE DEBUG_MODE

SOURCES += \
    modularcore.cpp

HEADERS += modularcore.h \
    global.h \
    module.h \
    moduleplugin.h \
    module-macros.h
