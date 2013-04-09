#-------------------------------------------------
#
# Project created by QtCreator 2013-03-28T17:53:37
#
#-------------------------------------------------

QT       += core

QMAKE_CXXFLAGS += -std=gnu++11 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fstack-protector-all -Wextra -Werror
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
