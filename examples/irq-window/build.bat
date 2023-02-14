@echo off
set ddkpath=V:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.31.31103
set path=%ddkpath%\bin\Hostx64\x64;V:\Program Files\Windows Kits\10\bin\10.0.22621.0\x64;%path%
set incpath=V:\Program Files\Windows Kits\10\Include\10.0.22621.0
set libpath=V:\Program Files\Windows Kits\10\Lib\10.0.22621.0

echo Compiling User Hypervisor...
cl .\main.c /I"%incpath%\shared" /I"%incpath%\um" /I"%incpath%\ucrt" /I"%ddkpath%\include" /I"%NOIRCVMAPI_PATH%\include" /Zi /nologo /W3 /WX /Od /Oi /D"_AMD64_" /D"_M_AMD64" /D"_WIN64" /D"_UNICODE" /D"UNICODE" /Zc:wchar_t /std:c17 /FAcs /Fa".\main.cod" /Fo".\main.obj" /Fd".\vc140.pdb" /GS- /Qspectre /TC /c /errorReport:queue

link .\main.obj /LIBPATH:"%libpath%\um\x64" /LIBPATH:"%libpath%\ucrt\x64" /LIBPATH:"%ddkpath%\lib\x64" /LIBPATH:"%NOIRCVMAPI_PATH%\lib\win11\chk\amd64" "NoirCvmApi_Static.lib" /NOLOGO /DEBUG /INCREMENTAL:NO /PDB:".\IrqWindowExample.pdb" /OUT:".\IrqWindowExample.exe" /SUBSYSTEM:CONSOLE /Machine:X64 /ERRORREPORT:QUEUE

echo Compiling Guest Program...
cl .\guest\guest.c /Zi /nologo /W3 /WX /Od /Oi /Zc:wchar_t /std:c17 /FAcs /Fa".\guest\guest.cod" /Fo".\guest\guest.obj" /Fd".\vc140.pdb" /GS- /Qspectre /TC /c /errorReport:queue

ml64 /W3 /WX /Zf /Zd /Fo".\guest\interrupt.obj" /c /nologo .\guest\interrupt.asm

link .\guest\*.obj /NOLOGO /NODEFAULTLIB /DEBUG /INCREMENTAL:NO /PDB:".\guest.pdb" /OUT:".\IrqWindowGuest.exe" /ENTRY:"guest_entry" /SUBSYSTEM:NATIVE /Machine:X64 /ERRORREPORT:QUEUE

nasm .\guest\misc.asm -o .\misc.bin

echo Completed!
pause.