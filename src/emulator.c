#include <Windows.h>
#include <strsafe.h>
#include <NoirCvmApi.h>
#include "emulator.h"

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
		// Adjust Increment of rsi/rdi according to rflags.df bit and operand size.
		LONG64 Increment=ExitContext->Io.Access.OperandSize*(_bittest64(&ExitContext->Rflags,10)?-1:1);
		// Use Memory to I/O.
		NOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess;
		// Translate Addresses before I/O. Note that x86 allows unaligned memory accesses.
		ULONG64 Gpa1,Gpa2=0;
		USHORT CopySize1,CopySize2=0;
		ULONG64 Gva=IoPortAccess.Direction?ExitContext->Io.Rdi:ExitContext->Io.Rsi;
		NOIR_TRANSLATE_GVA_FLAGS TranslationFlags=IoPortAccess.Direction?CvTranslateGvaFlagWrite:CvTranslateGvaFlagRead;
		NOIR_TRANSLATE_GVA_RESULT TranslationResult;
		if(ExitContext->VpState.Cpl==3)TranslationFlags|=CvTranslateGvaFlagUser;
		st=EmulatorCallbacks->TranslationCallback(Context,PAGE_BASE(Gva),TranslationFlags,&TranslationResult,&Gpa1);
		if(st!=NOIR_SUCCESS)goto TranslationFailed;
		// Translation may result in page faults.
		if(!TranslationResult.Successful)
		{
			st=EmulatorCallbacks->InjectionCallback(Context,CvPageFault,TRUE,(ULONG32)TranslationResult.Value);
			if(st!=NOIR_SUCCESS)goto InjectionFailed;
			// No need to I/O if there's page fault.
			goto IoSuccess;
		}
		Gpa1+=PAGE_OFFSET(Gva);
		CopySize1=IoPortAccess.AccessSize;
		// Unaligned memory accesses may overflow the page.
		if(PAGE_OVERFLOW(Gva,IoPortAccess.AccessSize))
		{
			st=EmulatorCallbacks->TranslationCallback(Context,NEXT_PAGE_BASE(Gva),0,&TranslationResult,&Gpa2);
			if(st!=NOIR_SUCCESS)goto InjectionFailed;
			// Translation may result in page faults.
			if(!TranslationResult.Successful)
			{
				st=EmulatorCallbacks->InjectionCallback(Context,CvPageFault,TRUE,(ULONG32)TranslationResult.Value);
				if(st!=NOIR_SUCCESS)goto InjectionFailed;
				// No need to I/O if there's page fault.
				goto IoSuccess;
			}
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

void static NoirPrintMemoryAccessInstruction(IN PNOIR_CVM_EXIT_CONTEXT ExitContext)
{
	CHAR RawByteString[48];
	BYTE i=0;
	for(;i<15;i++)StringCbPrintfA(&RawByteString[i*3],sizeof(RawByteString)-i*3,"%02X ",ExitContext->MemoryAccess.InstructionBytes[i]);
	NoirDebugPrint("Rip=0x%016llX, Instruction Raw Bytes: %s\n",ExitContext->Rip,RawByteString);
}

NOIR_STATUS NoirTryMmioEmulation(IN PNOIR_CVM_EMULATOR_CALLBACKS EmulatorCallbacks,IN OUT PVOID Context,IN PNOIR_CVM_EXIT_CONTEXT ExitContext,OUT PNOIR_EMULATION_STATUS ReturnStatus)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ReturnStatus->Value=0;
	if(!ExitContext->MemoryAccess.Flags.Decoded)
	{
		NoirDebugPrint("This memory access is not decoded by NoirVisor! Enable the decoder in vCPU options!\n");
		goto NonDecoded;
	}
	switch(ExitContext->MemoryAccess.Flags.InstructionCode)
	{
		case NoirCvmInstructionCodeMov:
		{
			NOIR_CVM_REGISTER_NAME RipName=NoirCvmRegisterRip;
			NOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess;
			MemoryAccess.Gpa=ExitContext->MemoryAccess.Gpa;
			MemoryAccess.Direction=ExitContext->MemoryAccess.Access.Write;
			MemoryAccess.AccessSize=(USHORT)ExitContext->MemoryAccess.Flags.OperandSize;
			switch(ExitContext->MemoryAccess.Flags.OperandClass)
			{
				case NoirCvmOperandClassGpr:
				case NoirCvmOperandClassGpr8Hi:
				{
					BYTE GprVal[8];
					NOIR_CVM_REGISTER_NAME MmioRegisterName=NoirCvmRegisterRax+ExitContext->MemoryAccess.Flags.OperandCode;
					if(MemoryAccess.Direction)
					{
						// This is output, so read from registers.
						st=EmulatorCallbacks->ViewRegistersCallback(Context,&MmioRegisterName,1,8,GprVal);
						if(st!=NOIR_SUCCESS)goto ViewRegistersFailed;
						// Copy but differentiate the operand class.
						if(ExitContext->MemoryAccess.Flags.OperandClass==NoirCvmOperandClassGpr8Hi)
							MemoryAccess.Data[0]=GprVal[1];
						else
							*(PULONG64)MemoryAccess.Data=*(PULONG64)GprVal;
					}
					st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
					if(st!=NOIR_SUCCESS)goto MmioFailed;
					if(!MemoryAccess.Direction)
					{
						// This in input, so write to registers.
						// Copy but differentiate the operand class.
						if(ExitContext->MemoryAccess.Flags.OperandClass==NoirCvmOperandClassGpr8Hi)
							GprVal[1]=MemoryAccess.Data[0];
						else
						{
							__movsb(GprVal,MemoryAccess.Data,MemoryAccess.AccessSize);
							// If the input operand is 32-bit, clear high 32-bits.
							if(MemoryAccess.AccessSize==4)*(PULONG32)&GprVal[4]=0;
						}
						st=EmulatorCallbacks->EditRegistersCallback(Context,&MmioRegisterName,1,8,GprVal);
						if(st!=NOIR_SUCCESS)goto EditRegistersFailed;
					}
					break;
				}
				case NoirCvmOperandClassImmediate:
				{
					// Only possible for outputs.
					*(PULONG64)MemoryAccess.Data=ExitContext->MemoryAccess.Operand.Immediate.u;
					st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
					if(st!=NOIR_SUCCESS)goto MmioFailed;
					break;
				}
				case NoirCvmOperandClassSegSel:
				{
					SEGMENT_REGISTER Seg;
					NOIR_CVM_REGISTER_NAME MmioRegisterName=NoirCvmRegisterEs+ExitContext->MemoryAccess.Flags.OperandCode;
					if(MemoryAccess.Direction)
					{
						st=EmulatorCallbacks->ViewRegistersCallback(Context,&MmioRegisterName,1,sizeof(Seg),&Seg);
						if(st!=NOIR_SUCCESS)goto ViewRegistersFailed;
						*(PUSHORT)MemoryAccess.Data=Seg.Selector;
					}
					st=EmulatorCallbacks->MemoryCallback(Context,&MemoryAccess);
					if(st!=NOIR_SUCCESS)goto MmioFailed;
					if(!MemoryAccess.Direction)
					{
						// This is input, so write to registers.
						Seg.Selector=*(PUSHORT)MemoryAccess.Data;
						st=EmulatorCallbacks->EditRegistersCallback(Context,&MmioRegisterName,1,sizeof(Seg),&Seg);
						if(st!=NOIR_SUCCESS)goto EditRegistersFailed;
					}
					break;
				}
				default:
				{
					NoirDebugPrint("Unknown operand class in mov instruction!\n",ExitContext->MemoryAccess.Flags.OperandClass);
					NoirPrintMemoryAccessInstruction(ExitContext);
					goto InternalFailure;
				}
			}
			// Edit Rip.
			st=EmulatorCallbacks->EditRegistersCallback(Context,&RipName,1,8,&ExitContext->NextRip);
			if(st!=NOIR_SUCCESS)goto EditRegistersFailed;
			goto MmioSuccess;
		}
		default:
		{
			NoirDebugPrint("Unknown MMIO Instruction Code: 0x%04X!\n",ExitContext->MemoryAccess.Flags.InstructionCode);
			NoirPrintMemoryAccessInstruction(ExitContext);
			goto InternalFailure;
		}
	}
MmioSuccess:
	ReturnStatus->EmulationSuccessful=1;
	return st;
MmioFailed:
	ReturnStatus->MemoryCallbackFailed=1;
	return st;
ViewRegistersFailed:
	ReturnStatus->ViewRegistersFailed=1;
	return st;
EditRegistersFailed:
	ReturnStatus->EditRegistersFailed=1;
	return st;
InternalFailure:
	ReturnStatus->InternalFailure=1;
	return st;
NonDecoded:
	ReturnStatus->UndecodedMmio=1;
	return st;
}