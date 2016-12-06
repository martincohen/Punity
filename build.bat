@echo off
setlocal enableextensions enabledelayedexpansion

set target=main
set target_c=%target%
set target_rc=%target%
set target_exe=%target%
set target_flag=PUN_TARGET_MAIN
set compiler=cl
set configuration=debug
set runtime=PUN_RUNTIME_WINDOWS
set platform=PUN_PLATFORM_WINDOWS
set luajit=0

if not exist bin mkdir bin
if not exist bin\examples mkdir bin\examples
if not exist bin\lunity mkdir bin\lunity

if exist import.bat (
	call "import"
)

for %%a in (%*) do (
	if "%%a"=="gcc" (
		set compiler=gcc
	) else if "%%a"=="cl" (
		set compiler=cl
	) else if "%%a"=="debug" (
		set configuration=debug
	) else if "%%a"=="release" (
		set configuration=release
	) else if "%%a"=="sdl" (
		set runtime=sdl
	) else if "%%a"=="luajit" (
		set luajit=1
	) else if "%%a"=="lunity" (
		set target_flag=PUN_TARGET_LUNITY
		set target=lunity/lunity
		set target_rc=lunity/lunity
	) else (
		set target=%%a
		set target_rc=%%a
	)
)

if not exist "%target_rc%.rc" (
	echo - WARNING: Set target resource file to `main.rc` as `%target_rc%.rc` was not found.
	set target_rc=main
)

set target_c=%target%.c
set target_rc=%target%

echo -------------------------------------------------
echo - Compiler:      %compiler%
echo - Configuration: %configuration%
echo - Runtime:       %runtime%
echo - Target C:      %target%
echo - Target RC:     %target_rc%.rc
echo - Output:        bin\%target%.exe
if "%luajit%"=="1" (
	echo + LuaJIT
)
echo -------------------------------------------------

if "%compiler%"=="cl" (
	rem MT for statically linked CRT, MD for dynamically linked CRT
	set win_runtime_lib=MD
	set common_c=..\!target_c! /Fe!target!.exe -nologo -D_WIN32_WINNT=0x0501 -D!runtime!=1 -D!platform!=1 -D!target_flag!=1 -TC -FC -EHa- -I..\ -I..\lib
	set common_l=/OPT:REF notelemetry.obj user32.lib gdi32.lib winmm.lib !target!.res

	if "%target%"=="lunity/lunity" (
		if "%luajit%"=="1" (
			set common_c=..\lib\stb_vorbis.c !common_c! -I..\lib\luajit\src
			set common_l=!common_l! -LIBPATH:..\lib\luajit\src lua51.lib
			if not exist bin\lunity\lua51.dll copy lib\luajit\src\lua51.dll bin\lunity\
		) else (
			set common_c=..\lib\stb_vorbis.c ..\lib\lua51\*.c !common_c! -I..\lib\lua51
		)
	)

	if "%runtime%"=="sdl" (
		set common_c=!common_c! -DPUN_RUNTIME_SDL=1 -I..\lib\SDL\include
		set common_l=/SUBSYSTEM:WINDOWS /INCREMENTAL:NO !common_l! -LIBPATH:..\lib\SDL\lib\x86 SDL2main.lib SDL2.lib opengl32.lib
		if not exist bin\SDL2.dll copy lib\SDL\lib\x86\SDL2.dll bin\
	) else (
		set common_c=!common_c! -DPUN_RUNTIME_WINDOWS=1
	)

	rem if "%configuration%"=="release" (
	rem 	echo.
	rem 	echo Building release shaders...
	rem 	fxc /nologo /T ps_4_0 /Fo res/default.cso res/default.hlsl
	rem ) else (
	rem 	echo.
	rem 	echo Building debug shaders...
	rem 	fxc /nologo /Od /Zi /T ps_4_0 /Fo res/default.cso res/default.hlsl
	rem )

	echo.
	echo Building resources...
	rc /nologo /fo bin\!target!.res !target_rc!.rc

	echo.
	echo Compiling...
	pushd bin
	if "%configuration%"=="release" (
		echo Release...
		if "!win_runtime_lib!"=="MD" (
			echo Applying special compiler and linker options...
		)
		cl !common_c! -!win_runtime_lib! -O2 -DPUN_RELEASE_BUILD=1 -Gy -Oy /link !common_l! 
		strip !target!.exe
	) else if "%configuration%"=="release_debug" (
		cl !common_c! -!win_runtime_lib! -O2 -DPUN_RELEASE_BUILD=1 -Gy -Oy -Z7 /link !common_l! 
	) else (
		cl !common_c! -!win_runtime_lib!d -Od -Z7 /link !common_l!
	)
	popd

) else if "%compiler%"=="gcc" (

	set common_c=!target_c! -std=gnu99 -o ./bin/!target!.exe ./bin/!target_rc!.o -D!runtime!=1 -D!platform!=1 -D!target_flag!=1 -D_WIN32_WINNT=0x0501 -I./lib -I./lib/mingw -I.
	set common_l=-luser32 -lgdi32 -lwinmm
	if "%target%"=="lunity/lunity" (
		if "%luajit%"=="1" (
			set common_c=./lib/stb_vorbis.c ./lib/luajit/src/luajit.o !common_c! -I./lib/luajit/src
			set common_l=-L./lib/luajit/src -llua51 !common_l!
			if not exist ./bin/lunity/lua51.dll copy ./lib/luajit/src/lua51.dll ./bin/lunity/
		) else (
			set common_c=./lib/stb_vorbis.c ./lib/lua51/*.c !common_c! -I./lib/lua51
		)
	)

	rem TODO: SDL

	echo.
	echo Building resources...
	windres -i !target_rc!.rc -o bin/!target_rc!.o

	echo.
	echo Compiling...
	if "%configuration%"=="release" (
		gcc !common_c! -O2 -DPUN_RELEASE_BUILD=1 -mwindows !common_l!
		echo Stripping...
		strip ./bin/!target!.exe
	) else (
		echo Building debug...
		gcc -g !common_c! !common_l!
	)

) else (
	echo Invalid compiler specified.
)

goto :end

:failed
echo.
echo Build failed.

:end