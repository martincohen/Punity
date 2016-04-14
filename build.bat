@echo off
set compiler=cl
setlocal enableextensions enabledelayedexpansion

if not exist bin mkdir bin

if "%2"=="gcc" (
	set compiler=gcc
)

if "%1"=="gcc" (
	set compiler=gcc
)

if "%compiler%"=="cl" (
	echo Using MSVC
	set common_c=%CD%\main.c -nologo -D_WIN32_WINNT=0x0501 -Gm- -TC -EHa- -MP -I..\ -I..\lib
	set common_l=/link /OPT:REF user32.lib gdi32.lib winmm.lib main.res /ignore:4068

	echo Building resources
	rc /nologo /fo bin\main.res main.rc

	pushd bin
	if "%1"=="release" (
		echo Building release
		cl !common_c! -O2 -MT -DRELEASE_BUILD /Gy /Oy !common_l!
		echo Stripping
		strip main.exe
	) else if "!1"=="released" (
		echo Building release with debug information
		cl !common_c! -O2 -MTd -DRELEASE_BUILD /Gy /Oy /Z7 !common_l!
	) else (
		echo Building debug
		cl !common_c! -Od -MDd -Z7 !common_l!
	)
	popd
) else if "%compiler%"=="gcc" (
	echo Using GCC
	set common=main.c -o ./bin/main.exe ./bin/main.o -D_WIN32_WINNT=0x0501 -I./lib -I./mingw -I. -luser32 -lgdi32 -lwinmm

	echo Building resources
	windres -i main.rc -o bin/main.o
	if "%1"=="release" (
		echo Building release
		gcc -O2 -DRELEASE_BUILD -mwindows %common%
		echo Stripping
		strip ./bin/main.exe
	) else (
		echo Building debug
		gcc -g %common%
	)
) else (
	echo Invalid compiler specified.
)
