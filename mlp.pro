QT -= core
QT += gui

TARGET = mlp

# DEFINES += ASSETS
DEFINES += ASSETS_DIR=\\\"$$PWD/\\\"

CONFIG -= qt
# CONFIG += c++11
CONFIG += console

QMAKE_CC= gcc -std=c99 -mwindows -g
#QMAKE_CFLAGS +=
#QMAKE_CFLAGS +=
#QMAKE_CFLAGS +=

SOURCES += \
    src/core/target.c \
    src/map.c \
    src/test.c

OTHER_FILES += \
    src/main.h \
    src/config.h \
    src/res.h \
    src/main.c \
    src/res.c \
    src/debug.c \
    src/world.c \
    src/core/core.c \
    src/core/core.h \
    src/core/assets.c \
    src/core/program.c

LIBS += -lmingw32

LIBS += -L$$PWD/lib/sdl/lib/ -lSDL2main
LIBS += -L$$PWD/lib/sdl/lib/ -lSDL2
INCLUDEPATH += $$PWD/lib/sdl/include
INCLUDEPATH += $$PWD/src

HEADERS += \
    src/world.h
