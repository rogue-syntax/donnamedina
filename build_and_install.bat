@echo off
rem Builds Release|x64 and installs it as C:\Program Files\Common Files\VST3\DonnaMedinaVST64.vst3
rem Usage: double-click, or run from any prompt. It asks for admin rights (needed for Program Files).
rem Close FL Studio (or any host using the plugin) first, otherwise the copy fails (file in use).
setlocal
cd /d "%~dp0"

rem --- re-run as administrator if needed
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
if not exist "%MSBUILD%" (
    echo MSBuild not found: %MSBUILD%
    goto :fail
)

echo.
echo === Building Release^|x64 ===
"%MSBUILD%" RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64 /p:PostBuildEventUseInBuild=false /m /v:minimal /nologo
if errorlevel 1 (
    echo Build FAILED.
    goto :fail
)

set SRC=%~dp0x64\Release\RSSynth001donnaMedina.vst3
set DESTDIR=%CommonProgramW6432%\VST3
set DEST=%DESTDIR%\DonnaMedinaVST64.vst3
set BACKUP=%DESTDIR%\DonnaMedinaVST64.vst3.original

if not exist "%DESTDIR%" mkdir "%DESTDIR%"

rem --- keep the first installed version once, never overwritten afterwards
if exist "%DEST%" if not exist "%BACKUP%" (
    echo Backing up the currently installed plugin to %BACKUP%
    copy /y "%DEST%" "%BACKUP%" >nul
)

echo.
echo === Installing ===
copy /y "%SRC%" "%DEST%"
if errorlevel 1 (
    echo Copy FAILED. Is FL Studio ^(or another host^) still running with the plugin loaded?
    goto :fail
)

echo.
echo Installed: %DEST%
for %%F in ("%DEST%") do echo   %%~tF  %%~zF bytes
echo Rescan plugins in FL Studio if it does not pick up the new version.
echo.
pause
exit /b 0

:fail
echo.
pause
exit /b 1
