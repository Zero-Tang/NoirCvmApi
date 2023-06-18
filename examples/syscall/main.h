#pragma once

#define VirtualMemorySize       0x00200000
#define VirtualMemoryPages      (VirtualMemorySize>>12)
#define IdtBase                 0x3000

typedef struct _PROGRAM_HEADER
{
    ULONG64 EntryPoint;
    ULONG64 SystemCallHandler;
    ULONG64 GdtBase;
    ULONG64 TssBase;
}PROGRAM_HEADER,*PPROGRAM_HEADER;

CVM_HANDLE VmHandle=0;
PVOID VirtualMemory=NULL;
PPROGRAM_HEADER ProgramHeader=NULL;
PCSTR GprNames[16]={"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"};