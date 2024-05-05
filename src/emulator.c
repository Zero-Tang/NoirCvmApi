#include <Windows.h>
#include <NoirCvmApi.h>
#include "emulator.h"

NOIR_STATUS static NoirThrowPageFault(IN PNOIR_CVM_EMULATOR_CALLBACKS EmulatorCallbacks,IN OUT PVOID Context,IN ULONG32 TranslationResult,OUT BOOL *Thrown)
{
	// FIXME: Implement Translation-Result to Page-Fault Error Code mechanism.
	NOIR_STATUS st=NOIR_SUCCESS;
	return st;
}

NOIR_STATUS NoirTryIoPortEmulation(IN PNOIR_CVM_EMULATOR_CALLBACKS EmulatorCallbacks,IN OUT PVOID Context,IN PNOIR_CVM_EXIT_CONTEXT ExitContext,OUT PNOIR_EMULATION_STATUS ReturnStatus)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	NOIR_EMULATOR_IO_ACCESS_INFO IoPortAccess;
	IoPortAccess.Direction=ExitContext->Io.Access.IoType;
	IoPortAccess.Port=ExitContext->Io.Port;
	IoPortAccess.AccessSize=ExitContext->Io.Access.OperandSize;
	ReturnStatus->Value=0;
	if(ExitContext->Io.Access.String)
	{
		// Returned Registers
		ULONG64 RegValues[4]=
		{
			ExitContext->Io.Rcx,
			ExitContext->Io.Rsi,
			ExitContext->Io.Rdi,
			ExitContext->Rip
		};
		LONG64 Increment=ExitContext->Io.Access.OperandSize*(_bittest64(&ExitContext->Rflags,10)?-1:1);
		// Use Memory to I/O.
		NOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess;
		// Translate Addresses before I/O. Note that x86 allows unaligned memory accesses.
		ULONG64 Gpa1,Gpa2=0;
		USHORT CopySize1,CopySize2=0;
		ULONG64 Gva=IoPortAccess.Direction?ExitContext->Io.Rdi:ExitContext->Io.Rsi;
		NOIR_TRANSLATE_GVA_FLAGS TranslationFlags=IoPortAccess.Direction?CvTranslateGvaFlagWrite:CvTranslateGvaFlagRead;
		ULONG32 TranslationResult;
		BOOL ExceptionThrown;
		if(ExitContext->VpState.Cpl==3)TranslationFlags|=CvTranslateGvaFlagUser;
		st=EmulatorCallbacks->TranslationCallback(Context,PAGE_BASE(Gva),TranslationFlags,&TranslationResult,&Gpa1);
		if(st!=NOIR_SUCCESS)goto TranslationFailed;
		// Translation may result in page faults.
		st=NoirThrowPageFault(EmulatorCallbacks,Context,TranslationResult,&ExceptionThrown);
		if(st!=NOIR_SUCCESS)goto InjectionFailed;
		if(ExceptionThrown)goto IoSuccess;
		Gpa1+=PAGE_OFFSET(Gva);
		CopySize1=IoPortAccess.AccessSize;
		// Unaligned memory accesses may overflow the page.
		if(PAGE_OVERFLOW(Gva,IoPortAccess.AccessSize))
		{
			st=EmulatorCallbacks->TranslationCallback(Context,NEXT_PAGE_BASE(Gva),0,&TranslationResult,&Gpa2);
			if(st!=NOIR_SUCCESS)goto InjectionFailed;
			// Translation may result in page faults.
			st=NoirThrowPageFault(EmulatorCallbacks,Context,TranslationResult,&ExceptionThrown);
			if(st!=NOIR_SUCCESS)goto TranslationFailed;
			if(ExceptionThrown)goto IoSuccess;
			CopySize2=(USHORT)(Gva+IoPortAccess.AccessSize-NEXT_PAGE_BASE(Gva));
			CopySize1-=CopySize2;
		}
		MemoryAccess.Gpa=Gpa1;
		MemoryAccess.Direction=IoPortAccess.Direction;
		MemoryAccess.AccessSize=CopySize1;
		if(!IoPortAccess.Direction)
		{
			// For Output, read memory before I/O.
			st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
			if(st!=NOIR_SUCCESS)goto MemoryFailed;
			__movsb(IoPortAccess.Data,MemoryAccess.Data,CopySize1);
			if(CopySize2)
			{
				MemoryAccess.Gpa=Gpa2;
				MemoryAccess.AccessSize=CopySize2;
				st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
				if(st!=NOIR_SUCCESS)goto MemoryFailed;
				__movsb(&IoPortAccess.Data[CopySize1],MemoryAccess.Data,CopySize2);
			}
		}
		// Perform I/O.
		st=EmulatorCallbacks->IoPortCallback(Context,&IoPortAccess);
		if(st!=NOIR_SUCCESS)goto IoFailed;
		if(IoPortAccess.Direction)
		{
			// For Input, write memory after I/O.
			__movsb(MemoryAccess.Data,IoPortAccess.Data,CopySize1);
			st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
			if(st!=NOIR_SUCCESS)goto MemoryFailed;
			if(CopySize2)
			{
				MemoryAccess.Gpa=Gpa2;
				MemoryAccess.AccessSize=CopySize2;
				__movsb(MemoryAccess.Data,&IoPortAccess.Data[CopySize1],CopySize2);
				st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
				if(st!=NOIR_SUCCESS)goto MemoryFailed;
			}
		}
		// Adjust registers.
		if(ExitContext->Io.Access.Repeat)
			RegValues[0]--;		// With repeat prefix, rcx will be decremented.
		// Input->es:rdi. Output->ds:rsi.
		RegValues[IoPortAccess.Direction+1]+=Increment;
		if(!ExitContext->Io.Access.Repeat || !RegValues[0])
			RegValues[3]=ExitContext->NextRip;
		st=EmulatorCallbacks->EditRegistersCallback(Context,StringIoRegisterNames,4,8,RegValues);
	}
	else
	{
		ULONG64 RegValues[2]=
		{
			ExitContext->Io.Rax,
			ExitContext->NextRip
		};
		// Use Register to I/O.
		*(PULONG32)IoPortAccess.Data=(ULONG32)ExitContext->Io.Rax;
		st=EmulatorCallbacks->IoPortCallback(Context,&IoPortAccess);
		if(st!=NOIR_SUCCESS)goto IoFailed;
		// If input, we need to edit rax register in vCPU.
		if(IoPortAccess.Direction)
			__movsb((PBYTE)RegValues,IoPortAccess.Data,IoPortAccess.AccessSize);
		// Finally, advance rip.
		st=EmulatorCallbacks->EditRegistersCallback(Context,RegisterIoRegisterNames,2,8,RegValues);
		if(st!=NOIR_SUCCESS)goto EditRegistersFailed;
	}
IoSuccess:
	// This goto route must be the first.
	ReturnStatus->EmulationSuccessful=1;
	return st;
EditRegistersFailed:
	ReturnStatus->EditRegistersFailed=1;
	return st;
InjectionFailed:
	ReturnStatus->InjectionFailed=1;
	return st;
TranslationFailed:
	ReturnStatus->TranslationFailed=1;
	return st;
MemoryFailed:
	ReturnStatus->MemoryCallbackFailed=1;
	return st;
IoFailed:
	ReturnStatus->IoPortCallbackFailed=1;
	return st;
}

NOIR_STATUS NoirTryMmioEmulation(IN PNOIR_CVM_EMULATOR_CALLBACKS EmulatorCallbacks,IN OUT PVOID Context,IN PNOIR_CVM_EXIT_CONTEXT ExitContext,OUT PNOIR_EMULATION_STATUS ReturnStatus)
{
	return NOIR_NOT_IMPLEMENTED;
}