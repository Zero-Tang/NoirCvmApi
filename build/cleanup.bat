@echo off

title NoirCvmApi Cleanup
echo Project: NoirCvmApi Library
echo Platform: Universal (Non-Binary Build)
echo Preset: Cleanup
echo Powered by zero.tangptr@gmail.com
echo Warning: All compiled binaries, including intermediate files, will be deleted!
pause.

echo Performing cleanup...
del ..\bin /q /s

if "%~1"=="/s" (echo Cleanup Completed!) else (pause)