@echo off
rem Builds and runs the SIMD voice prototype (standalone, no plugin code). Args: [voices] [seconds]
setlocal
set HERE=%~dp0
set OUT=%HERE%bin
if not exist "%OUT%" mkdir "%OUT%"
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" (echo vcvars64.bat not found: %VCVARS% & exit /b 1)
call "%VCVARS%" >nul || exit /b 1
rem same code generation as the plugin's Release build
cl /nologo /O2 /Ot /Ob2 /Oi /fp:fast /arch:AVX2 /EHsc /MD /Fo"%OUT%\\" /Fe"%OUT%\simd_proto.exe" "%HERE%simd_proto.cpp" || exit /b 1
"%OUT%\simd_proto.exe" %*
exit /b %errorlevel%
