@echo off
REM Builds GamerTCP-Setup.exe, the optional per-user installer.
REM The executable is built first, then embedded here together with the docs.
setlocal
cd /d "%~dp0.."

if not exist "GamerTCP.exe" (
    echo ERROR: GamerTCP.exe not found. Run build.bat first.
    exit /b 1
)
if not exist "README.md" (
    echo ERROR: README.md not found.
    exit /b 1
)
if not exist build mkdir build

REM Absolute paths so the resource compiler finds the payload from any cwd.
echo 101 RCDATA "%CD%\GamerTCP.exe" > "build\setup.rc"
echo 102 RCDATA "%CD%\README.md" >> "build\setup.rc"
echo 103 RCDATA "%CD%\CHANGES.md" >> "build\setup.rc"
echo 104 RCDATA "%CD%\LICENSE.md" >> "build\setup.rc"
echo 201 ICON "res\gamerTCP.ico" >> "build\setup.rc"

if /i "%~1"=="mingw" goto mingw

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto mingw
set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSDIR=%%i"
if not defined VSDIR goto mingw
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 goto mingw

rc /nologo /fo "build\setup.res" "build\setup.rc"
if errorlevel 1 exit /b 1

cl /nologo /EHsc /O2 /MT /W3 /std:c++17 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 ^
   /FeGamerTCP-Setup.exe /Fo:build\ installer\setup.cpp build\setup.res ^
   /link /SUBSYSTEM:CONSOLE ole32.lib shell32.lib advapi32.lib user32.lib uuid.lib
if errorlevel 1 exit /b 1

echo Built GamerTCP-Setup.exe with MSVC.
exit /b 0

:mingw
where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: no suitable compiler found. Install the Visual Studio C++ tools or MinGW-w64.
    exit /b 1
)

windres -O coff -i "build\setup.rc" -o "build\setup.res.o"
if errorlevel 1 exit /b 1

g++ -std=c++17 -O2 -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 ^
    -static -static-libgcc -static-libstdc++ ^
    installer\setup.cpp build\setup.res.o -o GamerTCP-Setup.exe ^
    -lole32 -lshell32 -ladvapi32 -luser32 -luuid
if errorlevel 1 exit /b 1

echo Built GamerTCP-Setup.exe with MinGW-w64.
exit /b 0
