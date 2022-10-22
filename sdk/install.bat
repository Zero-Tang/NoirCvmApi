@echo off

echo Constructing Library Environment...
mkdir .\lib\win7\chk\amd64
mkdir .\lib\win7\fre\amd64
mkdir .\lib\win11\chk\amd64
mkdir .\lib\win11\fre\amd64
setx NOIRCVMAPI_PATH %cd%

echo Copying Library Files...
copy ..\bin\compchk_win7x64\NoirCvmApi.lib .\lib\win7\chk\amd64\NoirCvmApi.lib
copy ..\bin\compfre_win7x64\NoirCvmApi.lib .\lib\win7\fre\amd64\NoirCvmApi.lib
copy ..\bin\compchk_win11x64\NoirCvmApi.lib .\lib\win11\chk\amd64\NoirCvmApi.lib
copy ..\bin\compchk_win11x64\NoirCvmApi_Static.lib .\lib\win11\chk\amd64\NoirCvmApi_Static.lib
copy ..\bin\compfre_win11x64\NoirCvmApi.lib .\lib\win11\fre\amd64\NoirCvmApi.lib
copy ..\bin\compfre_win11x64\NoirCvmApi_Static.lib .\lib\win11\fre\amd64\NoirCvmApi_Static.lib

echo Completed!
pause.