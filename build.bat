@echo off
REM GamerTCP build script. Prefers MSVC, falls back to MinGW-w64 g++.
setlocal
cd /d "%~dp0"

if not exist build mkdir build

if /i "%~1"=="mingw" goto mingw

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto mingw

set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSDIR=%%i"
if not defined VSDIR goto mingw

call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 goto mingw

pushd res
rc /nologo /d GAMERTCP_EMBED_MANIFEST /fo "..\build\gamerTCP.res" gamerTCP.rc
if errorlevel 1 (
    popd
    exit /b 1
)
popd

cl /nologo /EHsc /O2 /MT /W3 /std:c++17 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 ^
   /FegamerTCP.exe /Fo:build\ ^
   src\main.cpp src\engine.cpp src\cli.cpp src\gui.cpp build\gamerTCP.res ^
   /link /SUBSYSTEM:WINDOWS ^
   user32.lib advapi32.lib shell32.lib gdi32.lib comctl32.lib comdlg32.lib ^
   dwmapi.lib uxtheme.lib powrprof.lib
if errorlevel 1 exit /b 1

echo Built GamerTCP.exe with MSVC.
exit /b 0

:mingw
where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: no suitable compiler found. Install the Visual Studio C++ tools or MinGW-w64.
    exit /b 1
)

pushd res
windres -O coff -i gamerTCP.rc -o "..\build\gamerTCP.res.o"
if errorlevel 1 (
    popd
    exit /b 1
)
windres -O coff -i manifest_only.rc -o "..\build\default-manifest.o"
if errorlevel 1 (
    popd
    exit /b 1
)
popd

set "GTCBUILD=%CD%\build"
g++ -std=c++17 -O2 -Wall -Wextra -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 ^
    -mwindows -static -static-libgcc -static-libstdc++ -B"%GTCBUILD%/" ^
    src\main.cpp src\engine.cpp src\cli.cpp src\gui.cpp build\gamerTCP.res.o ^
    -o GamerTCP.exe ^
    -lcomctl32 -lcomdlg32 -ladvapi32 -lshell32 -lgdi32 -luser32 -ldwmapi -luxtheme -lpowrprof
if errorlevel 1 exit /b 1

echo Built GamerTCP.exe with MinGW-w64.
exit /b 0
