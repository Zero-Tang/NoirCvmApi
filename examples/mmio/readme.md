# MMIO Example
This directory stores the example that a User Hypervisor handles MMIO from CVM Guest.

## Memory Layout
The layout of guest memory space is defined as following:

| Address Start | Address End | Description |
|---|---|---|
| 0x000000 | 0x002FFF | Paging Structures |
| 0x003000 | 0x0030FF | GDT and TSS |
| 0x003100 | 0x003FFF | Stack |
| 0x004000 | 0x004FFF | IDT |
| 0x005000 | 0x1FFFFF | Program |
| 0x200000 | 0x3FFFFF | MMIO Space |

## MMIO Specification
In this example, MMIO range starts at 0x200000 and ends at 0x3FFFFF. \
All accesses must be 4-byte aligned, and less than the defined size. Otherwise, `#GP` exception will be thrown.

| Address | Size | Description |
|---|---|---|
| 0x200000 | 4 | Version of hardware. |
| 0x200004 | 4 | Read from or Write to console for a character. |
| 0x200008 | 4 | Writing `0x0000AA55` will shutdown the program. Reads from this address always return zero. |

## Build
To build, you must mount [EWDK11-22621](https://docs.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2022) to V: drive. \
Then run the `build.bat` script.

## Run
This program uses [PuTTY](https://www.putty.org) for simulated console.