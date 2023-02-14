#pragma once

#define VirtualPages		0x200
#define VirtualMemorySize	(VirtualPages<<12)

#define PagingBase				0x000000
#define GdtBase					0x003000
#define TssBase					0x003030
#define IdtBase					0x004000
#define ProgramBase				0x005000
#define StackTop				0x1FFFF8

CVM_HANDLE VmHandle;
HANDLE StdInHandle,StdOutHandle,StdErrHandle;
PVOID VirtualMemory;
ULONG64 EntryPointGva;