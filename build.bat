@echo off

set COMMON_FLAGS=-std=c99 -mwindows -I%~dp0. -lmingw32
set TOOL_FLAGS=%COMMON_FLAGS% -w -Wl,-subsystem,console
set MAIN_FLAGS=%COMMON_FLAGS% -I%~dp0lib/sdl/include -L%~dp0lib/sdl/lib -lSDL2main -lSDL2

echo ---------------------------------------------------------------------------
echo Compiling IDs
echo ---------------------------------------------------------------------------
gcc src/punity_id.c %TOOL_FLAGS% -o bin/id.exe
IF %ERRORLEVEL% NEQ 0 GOTO Errors

%~dp0bin\id.exe %~dp0src/ %~dp0src/id_enum.h


echo ---------------------------------------------------------------------------
echo Compiling assets
echo ---------------------------------------------------------------------------
gcc src/punity_assets.c %TOOL_FLAGS% -D_DEBUG -o bin/assets.exe
IF %ERRORLEVEL% NEQ 0 GOTO Errors

bin\assets.exe %~dp0/res
IF %ERRORLEVEL% NEQ 0 GOTO Errors

rem mv res\res.h src\res.h
rem mv res\res.c src\res.c

echo ---------------------------------------------------------------------------
echo Compiling program
echo ---------------------------------------------------------------------------

rem gcc src/punity_main.c -O2 -D_DEBUG %MAIN_FLAGS% -o bin/release/mlp.exe
gcc src/punity_main.c -g -DPATH_RES=\"\" -D_DEBUG %MAIN_FLAGS% -o bin/release/mlp.exe
IF %ERRORLEVEL% NEQ 0 GOTO Errors

copy /Y bin\mlp.exe package\win32\mlp.exe
copy /Y lib\SDL\bin\SDL2.dll package\win32\SDL2.dll
copy /Y res\res_d.bin package\win32\res_d.bin

rem strip bin\mlp.exe
IF %ERRORLEVEL% NEQ 0 GOTO Errors

echo ---------------------------------------------------------------------------
echo Done.
GOTO End

:Errors:
echo ---------------------------------------------------------------------------
echo Errors found.


:End