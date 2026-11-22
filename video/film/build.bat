@echo off
rem Builds the film tool (video\film\bin\film.exe) from the plugin's Release|x64 object files.
rem The tool is never shipped; see docs\08-video.md. Build the plugin first (and again after
rem every plugin change, or the tool runs old code):
rem   MSBuild RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64
rem Usage: build.bat            (build only)
rem        build.bat song.mid   (build, then run: video\film\bin\film.exe song.mid ...)
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
set RES=%OBJ%\RSSynth001donnaMedina.res
if not exist "%RES%" (echo %RES% not found: build Release^|x64 of the plugin first. & exit /b 1)

rem same defines as the plugin's Release|x64 compile, plus NOMINMAX (see tests\note_off)
set DEFS=/DNDEBUG /DGLEW_STATIC /DWIN32=1 /DVSTGUI_DIRECT2D_SUPPORT=1 /DWINDOWS=1 /DRELEASE=1 /D_CRT_SECURE_NO_DEPRECATE=1 /DVSTPLUGIN=1 /D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING /D_VC80_UPGRADE=0x0710 /D_WINDLL /D_MBCS /DNOMINMAX
set INCS=/I"%SDK%" /I"%SDK%\vstgui4" /I"%SDK%\external\revent" /I"%PROJ%\external\glew\include"

cl /nologo /c /MD /O2 /Zi /EHsc /fp:fast /w %DEFS% %INCS% /Fo"%OUT%\\" ^
  "%HERE%film.cpp" ^
  "%SDK%\public.sdk\source\vst\hosting\eventlist.cpp" ^
  "%SDK%\public.sdk\source\vst\hosting\parameterchanges.cpp" ^
  "%SDK%\public.sdk\source\vst\hosting\hostclasses.cpp" ^
  "%SDK%\public.sdk\source\vst\hosting\pluginterfacesupport.cpp" || exit /b 1

rem every plugin object except the DLL entry point (response file: the list is too long for cmd)
set RSP=%OUT%\objs.rsp
if exist "%RSP%" del "%RSP%"
for %%f in ("%OBJ%\*.obj") do if /I not "%%~nxf"=="dllmain.obj" echo "%%~ff">>"%RSP%"

set LIBDIR=%SDK%\build\lib\Release
link /nologo /DEBUG /OUT:"%OUT%\film.exe" /SUBSYSTEM:CONSOLE /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib ^
  "%OUT%\film.obj" "%OUT%\eventlist.obj" "%OUT%\parameterchanges.obj" "%OUT%\hostclasses.obj" "%OUT%\pluginterfacesupport.obj" @"%RSP%" "%RES%" ^
  "%LIBDIR%\base.lib" "%LIBDIR%\sdk.lib" "%LIBDIR%\vstgui.lib" "%LIBDIR%\vstgui_uidescription.lib" "%LIBDIR%\vstgui_support.lib" "%LIBDIR%\vstgui_standalone.lib" ^
  "%PROJ%\external\glew\lib\x64\glew32s.lib" opengl32.lib windowscodecs.lib psapi.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib || exit /b 1

if "%~1"=="" (echo built %OUT%\film.exe & exit /b 0)
"%OUT%\film.exe" %*
exit /b %errorlevel%
