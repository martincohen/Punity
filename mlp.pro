QT -= core
QT += gui

TARGET = mlp

# DEFINES += ASSETS
DEFINES += ASSETS_DIR=\\\"$$PWD/\\\"

CONFIG -= qt
# CONFIG += c++11
CONFIG += console

QMAKE_CC= gcc
QMAKE_CFLAGS += -std=gnu99
QMAKE_CFLAGS += -mwindows

SOURCES += \
    src/core/target.c

OTHER_FILES += \
    src/main.h \
    src/config.h \
    src/res.h \
    src/main.c \
    src/res.c \
    src/core/core.c \
    src/core/core.h \
    src/core/assets.c \
    src/core/program.c

LIBS += -lmingw32

LIBS += -L$$PWD/lib/sdl/mingw/lib/ -lSDL2main
LIBS += -L$$PWD/lib/sdl/mingw/lib/ -lSDL2
INCLUDEPATH += $$PWD/lib/sdl/mingw/include
INCLUDEPATH += $$PWD/src
