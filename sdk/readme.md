# NoirCvmApi SDK
NoirVisor Customizable VM API Software Development Kit

## Install the SDK
Before installing the SDK, you must build the project. It implies that this repository must be cloned to local environment. \
Execute the `install.bat` to install the SDK. \
If you wish to uninstall the SDK, delete this repository and delete the `NOIRCVMAPI_PATH` environment variable.

## Linking
If you build NoirCvmApi with Windows 7 presets, there aren't static libraries. Only dynamic libraries are available in Windows 7 presets. \
Both dynamic and static libraries are available in Windows 10 presets. \
If you build your project with dynamic libraries, you will have to copy the DLL files with your project or to the system directory on your own.

## Headers
The header files of NoirCvmApi is available in the `include` directory. \
The environment variable `NOIRCVMAPI_PATH` is the current directory, which includes the header files and library files. You may utilize this environment variable in your project's build system (e.g.: Visual Studio, batch, etc.).