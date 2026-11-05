@echo off
rem Runs a command in the VS 2022 x64 environment (Windows SDK, MSVC libs, cmake, ninja)
rem with portable LLVM 20 first on PATH, which the ReXGlue SDK needs (clang/clang++).
rem usage: rexenv.cmd <command...>
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=G:\LLVM\clang+llvm-20.1.8-x86_64-pc-windows-msvc\bin;%PATH%"
%*
