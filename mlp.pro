QT -= core
QT += gui

TARGET = mlp

DEFINES += _DEBUG
DEFINES += ASSETS_DIR=\\\"$$PWD/res/\\\"

CONFIG -= qt
# CONFIG += c++11
CONFIG += console

QMAKE_CC= gcc -std=c99 -mwindows -g

SOURCES += \
    src/punity.c

# DESTDIR = $$PWD/bin

OTHER_FILES += \
    src/config.h \
    src/main.h \
    src/main.c \
    res/res.h \
    res/res.c \
    src/debug.c \
    src/test.c \
    src/assets.c \
    src/memory.h \
    src/memory.c \
    src/punity_assets.h \
    src/punity_assets.c \
    src/punity_core.h \
    src/punity_core.c \
    src/punity_main.c \
    src/punity_world.h \
    src/punity_world.c \
    src/punity_tiled.h \
    src/punity_tiled.c \

INCLUDEPATH += $$PWD/src

LIBS += -lmingw32

INCLUDEPATH += $$PWD/lib
INCLUDEPATH += $$PWD/lib/SDL/include
LIBS += -L$$PWD/lib/SDL/lib/ -lSDL2main
LIBS += -L$$PWD/lib/SDL/lib/ -lSDL2

