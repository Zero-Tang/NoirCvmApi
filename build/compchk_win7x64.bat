@echo off
set ddkpath=C:\WinDDK\7600.16385.1
set path=%ddkpath%\bin\x86\amd64;%ddkpath%\bin\x86;%path%
set incpath=%ddkpath%\inc
set libpath=%ddkpath%\lib
set binpath=..\bin\compchk_win7x64
set objpath=..\bin\compchk_win7x64\Intermediate

title Compiling NoirCvmApi, Checked Build, 64-Bit Windows (AMD64 Architecture)
echo Project: NoirCvmApi Library
echo Platform: 64-Bit Windows
echo Preset: Debug/Checked Build
echo Powered by zero.tangptr@gmail.com
echo Copyright (c) 2021-2022, zero.tangptr@gmail.com. All Rights Reserved.
if "%~1"=="/s" (echo DO-NOT-PAUSE is activated!) else (pause)

echo ============Start Compiling============
cl ..\src\win\misc.c /I"%incpath%\api" /I"%incpath%\crt" /Zi /nologo /W3 /WX /Od /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /FAcs /Fa"%objpath%\misc.cod" /Fo"%objpath%\misc.obj" /Fd"%objpath%\vc90.pdb" /GS- /TC /c /errorReport:queue

cl ..\src\win\drv_comm.c /I"%incpath%\api" /I"%incpath%\crt" /Zi /nologo /W3 /WX /Od /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /FAcs /Fa"%objpath%\drv_comm.cod" /Fo"%objpath%\drv_comm.obj" /Fd"%objpath%\vc90.pdb" /GS- /TC /c /errorReport:queue

cl ..\src\win\public.c /I"%incpath%\api" /I"%incpath%\crt" /Zi /nologo /W3 /WX /Od /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /FAcs /Fa"%objpath%\public.cod" /Fo"%objpath%\public.obj" /Fd"%objpath%\vc90.pdb" /GS- /TC /c /errorReport:queue

rc /d"_AMD64_" /i"%incpath%\api" /i"%incpath%\crt" /fo"%objpath%\version.res" /n ..\src\win\version.rc

echo ============Start Linking============
link "%objpath%\misc.obj" "%objpath%\drv_comm.obj" "%objpath%\public.obj" "%objpath%\version.res" /LIBPATH:"%libpath%\win7\amd64" /LIBPATH:"%libpath%\Crt\amd64" /NODEFAULTLIB "kernel32.lib" "msvcrt.lib" /NOLOGO /DEBUG /INCREMENTAL:NO /NOENTRY /DEF:"..\src\win\export.def" /PDB:"%objpath%\NoirCvmApi.pdb" /OUT:"%binpath%\NoirCvmApi.dll" /SUBSYSTEM:WINDOWS /DLL /Machine:X64 /ERRORREPORT:QUEUE

if "%~1"=="/s" (echo Completed!) else (pause)