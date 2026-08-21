@echo off
REM GameTCP3 - Teredo Configuration
REM Copyright (c) 2022 Charles McDonald. BSD 3-Clause License. See LICENSE.md.
echo =============================
echo Teredo Configuration
echo =============================
echo.

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Administrator privileges required.
    echo Please right-click and select "Run as administrator".
    pause
    exit /B 1
)

echo This script configures Teredo tunneling.
echo Choose an option:
echo.
echo   1. Disable Teredo (recommended for gaming)
echo   2. Enable Teredo with default settings
echo   3. Enable Teredo with enterprise client
echo   4. Show current Teredo status
echo.
set /p "choice=Enter choice (1-4): "

if "%choice%"=="1" goto disable
if "%choice%"=="2" goto enable_default
if "%choice%"=="3" goto enable_enterprise
if "%choice%"=="4" goto show_status
echo.
echo Invalid choice. Please enter 1, 2, 3, or 4.
exit /B 1

:disable
echo.
echo Disabling Teredo...
netsh int teredo set state disabled
if %errorlevel% neq 0 (
    echo ERROR: Failed to disable Teredo.
    exit /B 1
) else (
    echo Teredo has been disabled successfully.
)
goto show_status

:enable_default
echo.
echo Enabling Teredo with default settings...
netsh int teredo set state type=default
if %errorlevel% neq 0 (
    echo ERROR: Failed to enable Teredo with default settings.
    exit /B 1
) else (
    echo Teredo has been enabled with default settings.
)
goto show_status

:enable_enterprise
echo.
echo Enabling Teredo with enterprise client...
netsh int teredo set state enterpriseclient
if %errorlevel% neq 0 (
    echo ERROR: Failed to enable Teredo with enterprise client.
    exit /B 1
) else (
    echo Teredo has been enabled with enterprise client.
)
goto show_status

:show_status
echo.
echo Current Teredo status:
echo ----------------------
netsh int teredo show state
if %errorlevel% neq 0 (
    echo ERROR: Failed to query Teredo status.
    exit /B 1
)

exit /B 0
