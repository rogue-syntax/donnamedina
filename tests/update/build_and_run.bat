@echo off
rem Builds and runs the update check tests (store spec licensing-v1.md 1.8; docs/07-licensing.md).
rem Self-contained: compiles source\updateCheck.cpp, source\license.cpp and Monocypher with release
rem settings. Set UPDATE_LIVE=1 to include the network tests (one real check against the store,
rem with the spec's test computer ID, and the 5 s deadline).
setlocal
set HERE=%~dp0
set PROJ=%HERE%..\..
set OUT=%HERE%bin
if not exist "%OUT%" mkdir "%OUT%"

set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" (echo vcvars64.bat not found: %VCVARS% & exit /b 1)
call "%VCVARS%" >nul || exit /b 1

cl /nologo /MD /O2 /EHsc /W3 /DNDEBUG /utf-8 /Fo"%OUT%\\" /Fe"%OUT%\update_test.exe" ^
  "%HERE%update_test.cpp" "%PROJ%\source\updateCheck.cpp" "%PROJ%\source\license.cpp" ^
  "%PROJ%\external\monocypher\monocypher.c" "%PROJ%\external\monocypher\monocypher-ed25519.c" ^
  advapi32.lib user32.lib || exit /b 1

"%OUT%\update_test.exe" %*
exit /b %errorlevel%
