@echo off
setlocal
pushd "%~dp0"
if not exist build mkdir build

:: Every step is checked and the failure path exits 1. This used to end on a
:: bare popd, which sets ERRORLEVEL to 0, so a failed compile reported success
:: and CI went green on a build that produced nothing.
rc.exe /fo build\pingy.res src\pingy.rc
if errorlevel 1 goto :fail

:: DEMOSCENE BUILD: Zero CRT, custom entry point, custom heap, custom math.
:: Every optimization flag known to man, in service of a Direct2D app that
:: links nothing it didn't write. Was it worth it? No. Do it anyway.
cl.exe /std:c++17 /O1 /GR- /GS- /Zl /W3 ^
  /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /DWIN32_LEAN_AND_MEAN /DNDEBUG ^
  /c /Fobuild\crt_mini.obj src\crt_mini.cpp
if errorlevel 1 goto :fail

cl.exe /std:c++17 /O1 /Gy /GL /GR- /GS- /Zl /W3 ^
  /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /DWIN32_LEAN_AND_MEAN /DNDEBUG ^
  /Fobuild\ ^
  src\main.cpp src\app.cpp src\renderer.cpp src\ping_worker.cpp src\ping_manager.cpp ^
  src\settings_io.cpp src\layout.cpp src\ui_sidebar.cpp src\ui_settings.cpp ^
  src\ui_add_dialog.cpp src\ui_text_input.cpp src\graph.cpp ^
  /link /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:_entry ^
  /OUT:build\Pingy.exe /LTCG /OPT:REF /OPT:NOICF /MERGE:.rdata=.text ^
  build\crt_mini.obj build\pingy.res ^
  kernel32.lib user32.lib gdi32.lib d2d1.lib dwrite.lib ^
  iphlpapi.lib ws2_32.lib ole32.lib shell32.lib
if errorlevel 1 goto :fail

:: The linker can report success and still leave nothing behind if the output
:: path was not writable. Cheap to check, and it is the thing anyone actually
:: cares about.
if not exist build\Pingy.exe (
  echo BUILD FAILED: no build\Pingy.exe
  goto :fail
)

echo.
echo Built build\Pingy.exe
popd
exit /b 0

:fail
echo.
echo BUILD FAILED
popd
exit /b 1
