@ECHO OFF
REM GameTCP3 - Build Script
REM Copyright (c) 2022 Charles McDonald. BSD 3-Clause License. See LICENSE.md.

setlocal enabledelayedexpansion

echo =============================
echo GameTCP3 - Build Script
echo =============================
echo.

if not exist "GameTCP3.vb" (
    echo ERROR: GameTCP3.vb not found in %cd%
    exit /B 1
)

where vbc >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Visual Basic compiler ^(^vbc^) not found in PATH.
    echo Please install Visual Studio or the Build Tools for Visual Studio.
    exit /B 1
)

echo Compiling GameTCP3.vb...
vbc /target:exe /platform:x64 /optimize+ /nologo /out:GameTCP3.exe GameTCP3.vb
if !errorlevel! neq 0 (
    echo ERROR: Failed to compile GameTCP3.vb
    exit /B 1
)
echo GameTCP3.exe compiled successfully.

echo.
if not exist "GameTCP3.exe.manifest" (
    echo INFO: No GameTCP3.exe.manifest found, skipping manifest embedding.
    goto buildDone
)

where mt >nul 2>&1
if !errorlevel! neq 0 (
    echo WARNING: mt.exe not found in PATH, skipping manifest embedding.
    goto buildDone
)

echo Embedding manifest into GameTCP3.exe...
mt.exe -nologo -manifest ".\GameTCP3.exe.manifest" -outputresource:".\GameTCP3.exe;#1"
if !errorlevel! neq 0 (
    echo WARNING: Failed to embed manifest into GameTCP3.exe
) else (
    echo Manifest embedded into GameTCP3.exe.
)

:buildDone
echo.
echo Build complete.
echo.
echo GameTCP3.exe - Comprehensive network optimization ^<requires admin^>.
echo.
echo Note: GameTCP3.exe requires the embedded manifest to run as Administrator.
