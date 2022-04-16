# NoirCvmApi
NoirVisor Customizable VM API Library

## Introduction
[NoirVisor](https://github.com/Zero-Tang/NoirVisor) is a hardware-accelerated hypervisor solution. This repository is a library project that abstracts the Customizable VM feature of NoirVisor so the functionalities may be exposed to any arbitrary users.

## Supported Platforms
Currently, only 64-bit Windows Operating Systems running on processors that support AMD-V are supported.

## Documentation
NoirCvmApi is currently in early stage of development. Documentation is released on [GitHub Wiki of this repository](https://github.com/Zero-Tang/NoirCvmApi/wiki).

## Build
There are two compatibility-concerned versions of scripted compilation. \
Before you execute any of compilation scripts, you must execute `build_prep.bat` to create the folders in order to hold the compiled executables.

### Aggressive Compatibility Option
This version literally means that it will compile regardless of compatibility. Empirically speaking, the eldest version the compiled library can support is Windows 7. \
Execute `compchk_win10x64.bat` to compile without optimization or `compfre_win10x64.bat` for compilation with optimization. \
To build NoirCvmApi with this option, you must mount [EWDK11-22000](https://docs.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2019-New) to T: drive.

### Retro-compatibility Option
This version will try to catch as older versions of systems as possible. This version would also generate smaller executables. \
Execute `compchk_win7x64.bat` to compile without optimization or `compfre_win7x64.bat` for compilation with optimization. \
To build NoirCvmApi with this option, you must install [WDK7.1-7600](https://www.microsoft.com/en-us/download/details.aspx?id=11800) to default location on C: drive.

## Importing NoirCvmApi to Your Project
**TODO**: Decision about the standard procedure for importing NoirCvmApi is not publicly available yet.

## License
This repository is under the MIT license.