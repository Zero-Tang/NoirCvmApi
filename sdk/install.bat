@echo off

echo Constructing Library Environment...
mkdir .\lib\win7\chk\amd64
mkdir .\lib\win7\fre\amd64
mkdir .\lib\win10\chk\amd64
mkdir .\lib\win10\fre\amd64
setx NOIRCVMAPI_PATH %cd%

echo Copying Library Files...
copy ..\bin\compchk_win7x64\NoirCvmApi.lib .\lib\win7\chk\amd64\NoirCvmApi.lib
copy ..\bin\compfre_win7x64\NoirCvmApi.lib .\lib\win7\fre\amd64\NoirCvmApi.lib
copy ..\bin\compchk_win10x64\NoirCvmApi.lib .\lib\win10\chk\amd64\NoirCvmApi.lib
copy ..\bin\compchk_win10x64\NoirCvmApi_Static.lib .\lib\win10\chk\amd64\NoirCvmApi_Static.lib
copy ..\bin\compfre_win10x64\NoirCvmApi.lib .\lib\win10\fre\amd64\NoirCvmApi.lib
copy ..\bin\compfre_win10x64\NoirCvmApi_Static.lib .\lib\win10\fre\amd64\NoirCvmApi_Static.lib

echo Completed!
pause.