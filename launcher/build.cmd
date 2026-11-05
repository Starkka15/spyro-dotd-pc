@echo off
rem Builds the launcher: build.cmd spyro_dotd
rem Output: build\<game>\<game>_launcher.exe (static CRT, no redistributable needed).
setlocal
if "%~1"=="" (echo usage: build.cmd ^<game^> & exit /b 1)
set GAME=%~1
set ROOT=%~dp0
if not exist "%ROOT%games\%GAME%.h" (echo no games\%GAME%.h & exit /b 1)
call "%ROOT%..\tools\rexenv.cmd" >nul 2>&1
set OUT=%ROOT%build\%GAME%
if not exist "%OUT%" mkdir "%OUT%"
copy /y "%ROOT%games\%GAME%.h" "%OUT%\game_selected.h" >nul
pushd "%OUT%"
rc /nologo /I "%OUT%" /I "%ROOT%games" /I "%ROOT%..\res" /I "%ROOT%src" /fo launcher.res "%ROOT%src\launcher.rc" || goto fail
cl /nologo /O2 /MT /W3 /utf-8 /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS /I "%OUT%" /I "%ROOT%src" ^
   "%ROOT%src\launcher.c" "%ROOT%src\import.c" "%ROOT%src\config.c" launcher.res ^
   /Fe:%GAME%_launcher.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib ole32.lib comctl32.lib bcrypt.lib uuid.lib || goto fail
del /q *.obj launcher.res >nul 2>&1
popd
echo built %OUT%\%GAME%_launcher.exe
exit /b 0
:fail
popd
echo BUILD FAILED
exit /b 1
