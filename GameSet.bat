@echo off
REM GameTCP3 - Admin Launcher
REM Copyright (c) 2022 Charles McDonald. BSD 3-Clause License. See LICENSE.md.
echo =============================
echo Running Admin shell
echo =============================

:init
setlocal DisableDelayedExpansion
set "batchPath=%~0"
for %%k in (%0) do set batchName=%%~nk
set "vbsGetPrivileges=%temp%\OEgetPriv_%batchName%.vbs"
setlocal EnableDelayedExpansion

:checkPrivileges
NET FILE 1>NUL 2>NUL
if '%errorlevel%' == '0' ( goto gotPrivileges ) else ( goto getPrivileges )

:getPrivileges
if '%1'=='ELEV' (echo ELEV & shift /1 & goto gotPrivileges)
echo.
echo **************************************
echo Invoking UAC for Privilege Escalation
echo **************************************

echo Set UAC = CreateObject^("Shell.Application"^) > "%vbsGetPrivileges%"
echo args = "ELEV " >> "%vbsGetPrivileges%"
echo For Each strArg in WScript.Arguments >> "%vbsGetPrivileges%"
echo args = args ^& strArg ^& " "  >> "%vbsGetPrivileges%"
echo Next >> "%vbsGetPrivileges%"
echo UAC.ShellExecute "!batchPath!", args, "", "runas", 1 >> "%vbsGetPrivileges%"
"%SystemRoot%\System32\WScript.exe" "%vbsGetPrivileges%" %*
if exist "%vbsGetPrivileges%" del "%vbsGetPrivileges%" 1>nul 2>nul
exit /B 1

:gotPrivileges
setlocal & pushd .
cd /d "%~dp0"
if '%1'=='ELEV' (del "%vbsGetPrivileges%" 1>nul 2>nul  &  shift /1)

echo =============================
echo GameSet - GameTCP3 Launcher
echo =============================

if exist "GameTCP3.exe" (
    GameTCP3.exe
    set exitCode=!errorlevel!
    if !exitCode! neq 0 (
        echo.
        echo WARNING: GameTCP3.exe exited with error code !exitCode!.
    )
) else (
    echo ERROR: GameTCP3.exe not found in %cd%
    echo Please compile it first using Create.bat
    set exitCode=1
)

if exist "%vbsGetPrivileges%" del "%vbsGetPrivileges%" 1>nul 2>nul
pause
exit /B !exitCode!
