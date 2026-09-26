setlocal enableextensions enabledelayedexpansion

if "%1"=="release" (
	echo Building release shaders...
	fxc /nologo /T ps_4_0 /Fo res/default.cso res/default.hlsl
) else (
	echo Building debug shaders...
	fxc /nologo /Od /Zi /T ps_4_0 /Fo res/default.cso res/default.hlsl
)