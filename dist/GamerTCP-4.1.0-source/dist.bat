@echo off
REM GamerTCP distribution script.
REM Produces three separate deliverables in dist\:
REM   GamerTCP-<version>-source.zip    complete source, build and installer scripts
REM   GamerTCP-<version>-portable.zip  portable executable with the documentation
REM   GamerTCP-<version>-setup.exe     per-user setup installer
REM GamerTCP.exe and GamerTCP-Setup.exe are built on demand when missing.
setlocal enabledelayedexpansion
cd /d "%~dp0"

set "BUILT_EXE=0"
if not exist "GamerTCP.exe" (
    echo GamerTCP.exe not found; building it with build.bat...
    call "%~dp0build.bat"
    if errorlevel 1 (
        echo ERROR: building GamerTCP.exe failed.
        exit /b 1
    )
    set "BUILT_EXE=1"
)
if not exist "GamerTCP.exe" (
    echo ERROR: build.bat did not produce GamerTCP.exe.
    exit /b 1
)

if not exist "GamerTCP-Setup.exe" (
    echo GamerTCP-Setup.exe not found; building it with installer\build-installer.bat...
    call "%~dp0installer\build-installer.bat"
    if errorlevel 1 (
        echo ERROR: building GamerTCP-Setup.exe failed.
        exit /b 1
    )
) else if "!BUILT_EXE!"=="1" (
    echo Rebuilding GamerTCP-Setup.exe to embed the rebuilt executable...
    call "%~dp0installer\build-installer.bat"
    if errorlevel 1 (
        echo ERROR: rebuilding GamerTCP-Setup.exe failed.
        exit /b 1
    )
)
if not exist "GamerTCP-Setup.exe" (
    echo ERROR: installer\build-installer.bat did not produce GamerTCP-Setup.exe.
    exit /b 1
)
if not exist "CHANGES.md" (
    echo ERROR: CHANGES.md not found.
    exit /b 1
)

REM The version comes from the newest release heading in CHANGES.md, the same
REM value selftest compares against the compiled version.
set "VER="
for /f "usebackq tokens=2 delims=[]" %%v in (`findstr /b /c:"## [" "CHANGES.md"`) do (
    if not defined VER set "VER=%%v"
)
if not defined VER (
    echo ERROR: cannot read the release version from CHANGES.md.
    exit /b 1
)

REM Use the full path: some machines disable cmd's current-directory search.
"%~dp0GamerTCP.exe" selftest >nul 2>&1
if errorlevel 1 (
    echo ERROR: self test failed. Run "GamerTCP.exe selftest" for details.
    exit /b 1
)

set "NAME=GamerTCP-%VER%"
set "SOURCE_DIR=dist\%NAME%-source"
set "BINARY_DIR=dist\%NAME%"
set "SOURCE_ZIP=dist\%NAME%-source.zip"
set "BINARY_ZIP=dist\%NAME%-portable.zip"
set "SETUP_EXE=dist\%NAME%-setup.exe"
set "CHECKSUMS=dist\%NAME%-SHA256SUMS.txt"

if not exist "dist" mkdir "dist"
if exist "%SOURCE_DIR%" rmdir /s /q "%SOURCE_DIR%"
if exist "%BINARY_DIR%" rmdir /s /q "%BINARY_DIR%"
if exist "%SOURCE_ZIP%" del /q "%SOURCE_ZIP%"
if exist "%BINARY_ZIP%" del /q "%BINARY_ZIP%"
if exist "%SETUP_EXE%" del /q "%SETUP_EXE%"
if exist "%CHECKSUMS%" del /q "%CHECKSUMS%"

mkdir "%SOURCE_DIR%" 2>nul
if errorlevel 1 (
    echo ERROR: cannot create "%SOURCE_DIR%".
    exit /b 1
)
mkdir "%BINARY_DIR%" 2>nul
if errorlevel 1 (
    echo ERROR: cannot create "%BINARY_DIR%".
    exit /b 1
)

REM Source distribution: everything needed to rebuild both executables.
copy /y "build.bat"     "%SOURCE_DIR%\build.bat"     >nul
copy /y "dist.bat"      "%SOURCE_DIR%\dist.bat"      >nul
copy /y "README.md"     "%SOURCE_DIR%\README.md"     >nul
copy /y "CHANGES.md"    "%SOURCE_DIR%\CHANGES.md"    >nul
copy /y "LICENSE.md"    "%SOURCE_DIR%\LICENSE.md"    >nul
xcopy /e /i /y /q "src" "%SOURCE_DIR%\src" >nul
if errorlevel 1 (
    echo ERROR: copying src failed.
    exit /b 1
)
xcopy /e /i /y /q "res" "%SOURCE_DIR%\res" >nul
if errorlevel 1 (
    echo ERROR: copying res failed.
    exit /b 1
)
xcopy /e /i /y /q "installer" "%SOURCE_DIR%\installer" >nul
if errorlevel 1 (
    echo ERROR: copying installer failed.
    exit /b 1
)

REM Portable binary distribution: the executable plus the documentation.
copy /y "GamerTCP.exe" "%BINARY_DIR%\GamerTCP.exe" >nul
copy /y "README.md"    "%BINARY_DIR%\README.md"    >nul
copy /y "CHANGES.md"   "%BINARY_DIR%\CHANGES.md"   >nul
copy /y "LICENSE.md"   "%BINARY_DIR%\LICENSE.md"   >nul

powershell -NoProfile -Command "Compress-Archive -Path '%SOURCE_DIR%' -DestinationPath '%SOURCE_ZIP%' -Force" >nul 2>&1
if not exist "%SOURCE_ZIP%" (
    echo ERROR: cannot create "%SOURCE_ZIP%".
    exit /b 1
)
powershell -NoProfile -Command "Compress-Archive -Path '%BINARY_DIR%' -DestinationPath '%BINARY_ZIP%' -Force" >nul 2>&1
if not exist "%BINARY_ZIP%" (
    echo ERROR: cannot create "%BINARY_ZIP%".
    exit /b 1
)

REM Setup installer: the per-user installer built by installer\build-installer.bat.
copy /y "GamerTCP-Setup.exe" "%SETUP_EXE%" >nul
if errorlevel 1 (
    echo ERROR: cannot create "%SETUP_EXE%".
    exit /b 1
)

REM SHA-256 of the three deliverables, one "hash  file" line each.
powershell -NoProfile -Command "$out = @(); foreach ($f in @('%SOURCE_ZIP%','%BINARY_ZIP%','%SETUP_EXE%')) { $out += (Get-FileHash -Algorithm SHA256 -Path $f).Hash.ToLower() + '  ' + (Split-Path -Path $f -Leaf) }; [IO.File]::WriteAllText('%CHECKSUMS%', (($out -join [char]10) + [char]10))"
if not exist "%CHECKSUMS%" (
    echo ERROR: cannot create "%CHECKSUMS%".
    exit /b 1
)

echo Packaged source distribution:  %SOURCE_ZIP%
echo Packaged portable binary:      %BINARY_ZIP%
echo Packaged setup installer:      %SETUP_EXE%
echo Packaged checksums:            %CHECKSUMS%
exit /b 0
