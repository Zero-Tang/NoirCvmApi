@echo off
set ddkpath=C:\WinDDK\7600.16385.1
set path=%ddkpath%\bin\x86\amd64;%ddkpath%\bin\x86;%ProgramFiles%\NASM;%path%
set incpath=%ddkpath%\inc
set libpath=%ddkpath%\lib

echo Compiling...
cl .\main.c /I"%incpath%\api" /I"%incpath%\crt" /I"%NOIRCVMAPI_PATH%\include" /Zi /nologo /W3 /WX /Od /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /FAcs /Fa"main.cod" /Fo"main.obj" /Fd"vc90.pdb" /GS- /Gy /TC /c /errorReport:queue

nasm .\guest\program.asm -o program.bin -l program.lst

echo Linking...
link "*.obj" /LIBPATH:"%libpath%\win7\amd64" /LIBPATH:"%libpath%\Crt\amd64" /LIBPATH:"%NOIRCVMAPI_PATH%\lib\win7\chk\amd64" "NoirCvmApi.lib" /NOLOGO /DEBUG /INCREMENTAL:NO /PDB:"PioExample.pdb" /OUT:"PioExample.exe" /OPT:REF /SUBSYSTEM:CONSOLE /Machine:X64 /ERRORREPORT:QUEUE

echo Completed!