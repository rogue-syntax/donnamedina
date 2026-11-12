@echo off
rem Builds and runs the editor open/close cycle test against the plugin's Release|x64 object files.
rem Build the plugin first:  MSBuild RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64 /p:PostBuildEventUseInBuild=false
rem Usage: build_and_run.bat [cycles]   (default 20)
setlocal
set HERE=%~dp0
set PROJ=%HERE%..\..
set SDK=%PROJ%\..\..\..\..\..
set OBJ=%PROJ%\x64\Release
set OUT=%HERE%bin
if not exist "%OUT%" mkdir "%OUT%"

set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" (echo vcvars64.bat not found: %VCVARS% & exit /b 1)
call "%VCVARS%" >nul || exit /b 1

if not exist "%OBJ%\NVGView.obj" (echo Build Release^|x64 of the plugin first. & exit /b 1)

rem same defines as the plugin's Release|x64 compile, plus NOMINMAX (see tests\note_off)
set DEFS=/DNDEBUG /DGLEW_STATIC /DWIN32=1 /DVSTGUI_DIRECT2D_SUPPORT=1 /DWINDOWS=1 /DRELEASE=1 /D_CRT_SECURE_NO_DEPRECATE=1 /DVSTPLUGIN=1 /D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING /D_VC80_UPGRADE=0x0710 /D_WINDLL /D_MBCS /DNOMINMAX
set INCS=/I"%SDK%" /I"%SDK%\vstgui4" /I"%SDK%\external\revent" /I"%PROJ%\external\glew\include" /I"%PROJ%\external\libbcrypt\include"

cl /nologo /c /MD /O2 /Zi /EHsc /fp:fast /w %DEFS% %INCS% /Fo"%OUT%\\" ^
  "%HERE%editor_cycle_test.cpp" ^
  "%SDK%\public.sdk\source\vst\hosting\hostclasses.cpp" ^
  "%SDK%\public.sdk\source\vst\hosting\pluginterfacesupport.cpp" || exit /b 1

set RSP=%OUT%\objs.rsp
if exist "%RSP%" del "%RSP%"
for %%f in ("%OBJ%\*.obj") do if /I not "%%~nxf"=="dllmain.obj" echo "%%~ff">>"%RSP%"

rem the plugin's compiled resources (vstgui.uidesc, images) so VSTGUI can load the editor
set RES=%OBJ%\RSSynth001donnaMedina.res
if not exist "%RES%" (echo %RES% not found & exit /b 1)

set LIBDIR=%SDK%\build\lib\Release
link /nologo /DEBUG /OUT:"%OUT%\editor_cycle_test.exe" /SUBSYSTEM:CONSOLE /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib ^
  "%OUT%\editor_cycle_test.obj" "%OUT%\hostclasses.obj" "%OUT%\pluginterfacesupport.obj" @"%RSP%" "%RES%" ^
  "%LIBDIR%\base.lib" "%LIBDIR%\sdk.lib" "%LIBDIR%\vstgui.lib" "%LIBDIR%\vstgui_uidescription.lib" "%LIBDIR%\vstgui_support.lib" "%LIBDIR%\vstgui_standalone.lib" ^
  "%PROJ%\external\glew\lib\x64\glew32s.lib" opengl32.lib psapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib || exit /b 1

"%OUT%\editor_cycle_test.exe" %*
exit /b %errorlevel%
