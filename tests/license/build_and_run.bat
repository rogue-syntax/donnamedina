@echo off
rem Builds and runs the licensing tests (docs/07-licensing.md). Self-contained: compiles
rem source\license.cpp and Monocypher with release settings (no _DEBUG, so the built-in key
rem table is the release one).
setlocal
set HERE=%~dp0
set PROJ=%HERE%..\..
set OUT=%HERE%bin
if not exist "%OUT%" mkdir "%OUT%"

set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" (echo vcvars64.bat not found: %VCVARS% & exit /b 1)
call "%VCVARS%" >nul || exit /b 1

cl /nologo /MD /O2 /EHsc /W3 /DNDEBUG /utf-8 /Fo"%OUT%\\" /Fe"%OUT%\license_test.exe" ^
  "%HERE%license_test.cpp" "%PROJ%\source\license.cpp" ^
  "%PROJ%\external\monocypher\monocypher.c" "%PROJ%\external\monocypher\monocypher-ed25519.c" ^
  advapi32.lib user32.lib || exit /b 1

"%OUT%\license_test.exe" %*
exit /b %errorlevel%
