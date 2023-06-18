# System Call Example
This example demonstrates how the `syscall` instruction clears bits in `rflags` register.

## Build
To build, you must mount [EWDK11-22621](https://docs.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2022) to V: drive. \
You should install [NASM](https://www.nasm.us/pub/nasm/stable/win64/) to build guest program. Adding to `PATH` is not required. \
Finally, run the `build.bat` script.