@echo off
rem Builds the release package for the store: installer\output\DonnaMedina-<version>.zip (the installer
rem plus manifest.json), with the store's build.ps1 (README.md, spec 1.9). Build the plugin first
rem (Release|x64, see BUILDING.md). Needs Inno Setup 6.3+. Details: release.ps1.
rem Send the store owner the zip; output\DonnaMedinaSetup.exe is the installer alone, for testing.
setlocal
rem started from PowerShell 7, Windows PowerShell inherits 7's module path and loses Get-FileHash
set "PSModulePath="
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0release.ps1" %*
exit /b %errorlevel%
