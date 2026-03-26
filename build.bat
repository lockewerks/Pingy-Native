@echo off
pushd "%~dp0"
if not exist build mkdir build

rc.exe /fo build\pingy.res src\pingy.rc 2>nul

:: DEMOSCENE BUILD: Zero CRT, custom entry point, custom heap, custom math.
:: I brought every optimization flag known to man to produce a 16KB ping tool.
:: Was it worth it? The binary is smaller than this comment block, so yes.
cl.exe /std:c++17 /O1 /GR- /GS- /Zl /W3 ^
  /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /DWIN32_LEAN_AND_MEAN /DNDEBUG ^
  /c /Fobuild\crt_mini.obj src\crt_mini.cpp

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
popd
