#-------------------------------------------------
#
# Project created by QtCreator 2013-03-28T17:53:37
#
#-------------------------------------------------

QT       += core

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fstack-protector -march=native -mtune=native -flto=8 -fuse-linker-plugin -Ofast

QMAKE_LFLAGS += -Wl,--as-needed

LIBS    += -lz -lmagic

QT       -= gui

TARGET = MySQLAnalyser
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app



SOURCES += main.cpp \
    worker.cpp

HEADERS += \
    worker.h
