@echo off
echo ---------------------------------------------------------------------------
echo Compiling assets
echo ---------------------------------------------------------------------------
gcc -o bin/assets.exe -std=gnu99 src/core/target.c -DASSETS -I%~dp0 -I%~dp0src

bin\assets.exe
rm res.bin
mv res.h src\res.h
mv res.c src\res.c

echo ---------------------------------------------------------------------------
echo Compiling program
echo ---------------------------------------------------------------------------

rem ---------------------------------------------------------------------------
rem SDL static build:
rem ---------------------------------------------------------------------------
rem The magical trick is to delete or rename the file "libSDL2.dll.a"
rem http://stackoverflow.com/questions/17620884/static-linking-of-sdl2-libraries
rem set SDL_DEPS=-lSDL2main -lSDL2 -lwinmm -limm32 -lole32 -loleaut32 -lversion

set SDL_DEPS=-lSDL2main -lSDL2
gcc -o bin/mlp.exe -O2 src/core/target.c -std=gnu99 -mwindows -I%~dp0 -I%~dp0src -Ilib/sdl/include -Llib/sdl/lib -lmingw32 %SDL_DEPS%

strip bin\mlp.exe

echo ---------------------------------------------------------------------------
echo Done