@echo off

rem SDL static build:
rem The magical trick is to delete or rename the file "libSDL2.dll.a"
rem http://stackoverflow.com/questions/17620884/static-linking-of-sdl2-libraries
rem set SDL_DEPS=-lSDL2main -lSDL2 -lwinmm -limm32 -lole32 -loleaut32 -lversion

set SDL_DEPS=-lSDL2main -lSDL2
set MLP_FLAGS=-std=c99 src/punity.c -mwindows -I%~dp0. -I%~dp0src -I%~dp0lib/sdl/include -L%~dp0lib/sdl/lib -lmingw32 %SDL_DEPS%


echo ---------------------------------------------------------------------------
echo Compiling assets
echo ---------------------------------------------------------------------------
gcc -o bin/assets.exe -g -DASSETS %MLP_FLAGS%

bin\assets.exe %~dp0res/
rem mv res\res.h src\res.h
rem mv res\res.c src\res.c

echo ---------------------------------------------------------------------------
echo Compiling program
echo ---------------------------------------------------------------------------

gcc -o bin/mlp.exe -O2 %MLP_FLAGS%

strip bin\mlp.exe

echo ---------------------------------------------------------------------------
echo Done