#pragma once

#define VIRTUAL_MEMORY_SIZE		0x200000
#define INIT_STACK_POINTER		0x1FFFF0

typedef struct _PROGRAM_HEADER
{
	ULONG64 EntryPoint;
	ULONG64 IdtBase;
	ULONG64 GdtBase;
	ULONG64 TssBase;
	ULONG64 PagingBase;
	ULONG64 Reserved;
}PROGRAM_HEADER,*PPROGRAM_HEADER;

NOIR_STATUS HvIoPortCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_IO_ACCESS_INFO IoAccess);
NOIR_STATUS HvMemoryCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess);
NOIR_STATUS HvViewRegisterCallback(IN OUT PVOID Context,IN PNOIR_CVM_REGISTER_NAME RegisterNames,IN ULONG32 RegisterCount,IN ULONG32 RegisterSize,OUT PVOID RegisterValues);
NOIR_STATUS HvEditRegisterCallback(IN OUT PVOID Context,IN PNOIR_CVM_REGISTER_NAME RegisterNames,IN ULONG32 RegisterCount,IN ULONG32 RegisterSize,IN PVOID RegisterValues);
NOIR_STATUS HvTranslateGvaPageCallback(IN OUT PVOID Context,IN ULONG64 GvaPage,IN NOIR_TRANSLATE_GVA_FLAGS TranslationFlags,OUT PULONG32 TranslationResult,OUT PULONG64 GpaPage);
NOIR_STATUS HvInjectExceptionCallback(IN OUT PVOID Context,IN NOIR_CVM_EXCEPTION_VECTOR Vector,IN BOOL HasErrorCode,IN ULONG32 ErrorCode);

NOIR_CVM_EMULATOR_CALLBACKS EmulatorCallbacks=
{
	sizeof(NOIR_CVM_EMULATOR_CALLBACKS),
	0,
	HvIoPortCallback,
	HvMemoryCallback,
	HvViewRegisterCallback,
	HvEditRegisterCallback,
	HvTranslateGvaPageCallback,
	HvInjectExceptionCallback
};

CVM_HANDLE VmHandle;
PVOID VirtualMemory;