@echo off
set ddkpath=T:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.28.29910
set path=%ddkpath%\bin\Hostx64\x64;T:\Program Files\Windows Kits\10\bin\10.0.22000.0\x64;%path%
set incpath=T:\Program Files\Windows Kits\10\Include\10.0.22000.0
set libpath=T:\Program Files\Windows Kits\10\Lib\10.0.22000.0
set binpath=..\bin\compfre_win10x64
set objpath=..\bin\compfre_win10x64\Intermediate

title Compiling NoirCvmApi, Free Build, 64-Bit Windows (AMD64 Architecture)
echo Project: NoirCvmApi Library
echo Platform: 64-Bit Windows
echo Preset: Release/Free Build
echo Powered by zero.tangptr@gmail.com
echo Copyright (c) 2021, zero.tangptr@gmail.com. All Rights Reserved.
if "%~1"=="/s" (echo DO-NOT-PAUSE is activated!) else (pause)

echo ============Start Compiling============
cl ..\src\win\misc.c /I"%incpath%\shared" /I"%incpath%\um" /I"%incpath%\ucrt" /I"%ddkpath%\include" /Zi /nologo /W3 /WX /O2 /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /std:c17 /FAcs /Fa"%objpath%\misc.cod" /Fo"%objpath%\misc.obj" /Fd"%objpath%\vc140.pdb" /GS- /Gy /GF /Qspectre /TC /c /errorReport:queue

cl ..\src\win\drv_comm.c /I"%incpath%\shared" /I"%incpath%\um" /I"%incpath%\ucrt" /I"%ddkpath%\include" /Zi /nologo /W3 /WX /O2 /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /std:c17 /FAcs /Fa"%objpath%\drv_comm.cod" /Fo"%objpath%\drv_comm.obj" /Fd"%objpath%\vc140.pdb" /GS- /Gy /GF /Qspectre /TC /c /errorReport:queue

cl ..\src\win\public.c /I"%incpath%\shared" /I"%incpath%\um" /I"%incpath%\ucrt" /I"%ddkpath%\include" /Zi /nologo /W3 /WX /O2 /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /std:c17 /FAcs /Fa"%objpath%\public.cod" /Fo"%objpath%\public.obj" /Fd"%objpath%\vc140.pdb" /GS- /Gy /GF /Qspectre /TC /c /errorReport:queue

rc /nologo /i"%incpath%\shared" /i"%incpath%\um" /I"%incpath%\ucrt" /I"%ddkpath%\include" /d"_AMD64_" /fo"%objpath%\version.res" /n ..\src\win\version.rc

echo ============Start Linking============
link "%objpath%\misc.obj" "%objpath%\drv_comm.obj" "%objpath%\public.obj" "%objpath%\version.res" /LIBPATH:"%libpath%\um\x64" /LIBPATH:"%libpath%\ucrt\x64" /LIBPATH:"%ddkpath%\lib\x64" /NOLOGO /DEBUG /INCREMENTAL:NO /DEF:"..\src\win\export.def" /PDB:"%objpath%\NoirCvmApi.pdb" /OUT:"%binpath%\NoirCvmApi.dll" /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /DLL /Machine:X64 /ERRORREPORT:QUEUE

if "%~1"=="/s" (echo Completed!) else (pause)