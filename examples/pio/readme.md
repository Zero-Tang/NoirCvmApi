# PIO Example
This directory stores the example that a User Hypervisor handles Port I/O.

## Emulator API
Starting May 2024, NoirCvmApi provides a set of Emulator API that further decodes the I/O operations so that data can be transferred right-at-the-box. This example will utilize this set of API.

## Test Cases
In order to thoroughly test the Emulator API, the guest program will be completely written in the Assembly language. The test cases will include:

- Register Output: Including 8-bit, 16-bit and 32-bit test cases.
- Register Input: Retrieve input from User Hypervisor, process it, and send to output.
- String I/O: The I/O will be done through memory. This kind of testing mainly focuses on the direction of I/O, memory translations and exceptions.

This example currently does not care about the Port number.

## Emulated Hardware
This example emulates two hardware. One test-case hardware and one debug hardware.

| Hardware | Port Range |
|---|---|
| Test-Case | 0x00 to 0x03 |
| Debug | 0xE9 |

### Debug Output
The debug hardware works exactly like ISA-DebugCon in Bochs and QEMU. Any reads from the port will return 0xE9 and any writes to the port will be sent to the console. \
This example will use [PuTTY](https://www.putty.org) over named pipe for debug output.

## Build
To build this example, you must install [WDK7.1-7600](https://www.microsoft.com/en-us/download/details.aspx?id=11800) to default location on C: drive. You must also install [NASM](https://www.nasm.us/pub/nasm/stable/win64/) in order to build guest program. \
Then run the `build.bat` script.