@echo off
rem Builds installer\output\DonnaMedinaSetup.exe with the store's recipe (README.md, rogue-syntax.iss).
rem Build the plugin first (Release|x64, see BUILDING.md). Needs Inno Setup 6.3+ (ISCC.exe).
rem The version comes from source\version.h (the same number the plugin reports and the update check
rem sends); hand the store owner the .exe and that number.
setlocal
set HERE=%~dp0
set PROJ=%HERE%..
set VST=%PROJ%\x64\Release\RSSynth001donnaMedina.vst3
if not exist "%VST%" (echo %VST% not found: build Release^|x64 first. & exit /b 1)

set ISCC=
for %%p in ("%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" "%ProgramFiles%\Inno Setup 6\ISCC.exe" "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe") do if exist %%p set ISCC=%%~p
if not defined ISCC for %%i in (ISCC.exe) do set ISCC=%%~$PATH:i
if not defined ISCC (echo ISCC.exe not found: install Inno Setup 6.3 or newer. & exit /b 1)

set MAJ=
set SUB=
set REL=
for /f "tokens=2,3" %%a in ('findstr /b /c:"#define MAJOR_VERSION_STR" /c:"#define SUB_VERSION_STR" /c:"#define RELEASE_NUMBER_STR" "%PROJ%\source\version.h"') do (
  if "%%a"=="MAJOR_VERSION_STR" set MAJ=%%~b
  if "%%a"=="SUB_VERSION_STR" set SUB=%%~b
  if "%%a"=="RELEASE_NUMBER_STR" set REL=%%~b
)
if not defined REL (echo could not read the version from source\version.h & exit /b 1)
set VER=%MAJ%.%SUB%.%REL%

if not exist "%HERE%in" mkdir "%HERE%in"
copy /y "%VST%" "%HERE%in\DonnaMedinaVST64.vst3" >nul || exit /b 1
pushd "%HERE%"
"%ISCC%" /Q plugins\DonnaMedina.iss /DPluginVersion=%VER%
set RC=%errorlevel%
popd
if %RC% neq 0 exit /b %RC%
echo installer\output\DonnaMedinaSetup.exe  version %VER%
exit /b 0
