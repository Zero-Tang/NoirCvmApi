#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <NoirCvmApi.h>
#include "main.h"

void HvSendToDebugConsole(IN BYTE Data)
{
	DWORD dwWrite;
	WriteFile(PipeHandle,&Data,1,&dwWrite,NULL);
}

NOIR_STATUS HvIoPortCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_IO_ACCESS_INFO IoAccess)
{
	if(!IoAccess->Direction)
	{
		if(IoAccess->Port<=ISA_DEBUG_CON_PORT && IoAccess->Port+IoAccess->AccessSize>ISA_DEBUG_CON_PORT)
		{
			// Debug Port. Send to named pipe.
			HvSendToDebugConsole(IoAccess->Data[ISA_DEBUG_CON_PORT-IoAccess->Port]);
		}
		else if(IoAccess->Port==0)
		{
			// Output to Console.
			printf("Output Data: %.*s\n",IoAccess->AccessSize,IoAccess->Data);
		}
	}
	else
	{
		if(IoAccess->Port<=ISA_DEBUG_CON_PORT && IoAccess->Port+IoAccess->AccessSize>ISA_DEBUG_CON_PORT)
		{
			// Debug Port. Always return ISA-DebugCon signature byte.
			IoAccess->Data[ISA_DEBUG_CON_PORT-IoAccess->Port]=0xE9;
		}
		else if(IoAccess->Port==0)
		{
			IoAccess->Data[0]=InputSource[InputPointer++];
			if(InputPointer==sizeof(InputSource))InputPointer=0;
		}
	}
	return NOIR_SUCCESS;
}

NOIR_STATUS HvMemoryCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess)
{
	// Note that this callback does not necessarily mean Memory-Mapped I/O.
	if(MemoryAccess->Gpa+MemoryAccess->AccessSize<VIRTUAL_MEMORY_SIZE)
	{
		PVOID Hva=(PVOID)((ULONG64)VirtualMemory+MemoryAccess->Gpa);
		if(MemoryAccess->Direction)
			RtlCopyMemory(Hva,MemoryAccess->Data,MemoryAccess->AccessSize);
		else
			RtlCopyMemory(MemoryAccess->Data,Hva,MemoryAccess->AccessSize);
		return NOIR_SUCCESS;
	}
	return NOIR_UNSUCCESSFUL;
}

NOIR_STATUS HvViewRegisterCallback(IN OUT PVOID Context,IN PNOIR_CVM_REGISTER_NAME RegisterNames,IN ULONG32 RegisterCount,IN ULONG32 RegisterSize,OUT PVOID RegisterValues)
{
	return NoirViewVirtualProcessorRegister2(VmHandle,0,RegisterNames,RegisterCount,RegisterSize,RegisterValues);
}

NOIR_STATUS HvEditRegisterCallback(IN OUT PVOID Context,IN PNOIR_CVM_REGISTER_NAME RegisterNames,IN ULONG32 RegisterCount,IN ULONG32 RegisterSize,IN PVOID RegisterValues)
{
	return NoirEditVirtualProcessorRegister2(VmHandle,0,RegisterNames,RegisterCount,RegisterSize,RegisterValues);
}

NOIR_STATUS HvTranslateGvaPageCallback(IN OUT PVOID Context,IN ULONG64 GvaPage,IN NOIR_TRANSLATE_GVA_FLAGS TranslationFlags,OUT PNOIR_TRANSLATE_GVA_RESULT TranslationResult,OUT PULONG64 GpaPage)
{
	// This example only implements a range.
	if(GvaPage<VIRTUAL_MEMORY_SIZE)
	{
		*GpaPage=GvaPage;
		TranslationResult->Value=0;
		TranslationResult->Successful=1;
	}
	else
	{
		*GpaPage=0;
		TranslationResult->Value=0;
		TranslationResult->Write=(TranslationFlags&CvTranslateGvaFlagWrite)==CvTranslateGvaFlagWrite;
	}
	return NOIR_SUCCESS;
}

NOIR_STATUS HvInjectExceptionCallback(IN OUT PVOID Context,IN NOIR_CVM_EXCEPTION_VECTOR Vector,IN BOOL HasErrorCode,IN ULONG32 ErrorCode)
{
	puts("Exception-Injection Callback is called!");
	return NoirSetEventInjection(VmHandle,0,TRUE,Vector,NoirEventTypeException,0,HasErrorCode,ErrorCode);
}

void HvRunVirtualMachine()
{
	BOOLEAN ContinueExecution=TRUE;
	do
	{
		NOIR_CVM_EXIT_CONTEXT ExitContext;
		NOIR_STATUS st=NoirRunVirtualProcessor(VmHandle,0,&ExitContext);
		if(st==NOIR_SUCCESS)
		{
			switch(ExitContext.InterceptCode)
			{
				case CvIoInstruction:
				{
					NOIR_EMULATION_STATUS retst;
					st=NoirTryIoPortEmulation(&EmulatorCallbacks,NULL,&ExitContext,&retst);
					if(st!=NOIR_SUCCESS)
					{
						printf("Failed to emulate I/O instruction! Return-Status: 0x%llX\n",retst.Value);
						ContinueExecution=FALSE;
					}
					break;
				}
				case CvHltInstruction:
				{
					puts("The guest program halted execution!");
					ContinueExecution=FALSE;
					break;
				}
				case CvException:
				{
					printf("Exception is intercepted at rip=0x%llX!\n",ExitContext.Rip);
					printf("Vector=%u, Error Code=0x%X\n",ExitContext.Exception.Vector,ExitContext.Exception.ErrorCode);
					ContinueExecution=FALSE;
					break;
				}
				default:
				{
					printf("Unknown Intercept Code: 0x%X!\n",ExitContext.InterceptCode);
					ContinueExecution=FALSE;
					break;
				}
			}
		}
		else
		{
			printf("Failed to run vCPU! Status=0x%X\n",st);
			break;
		}
	}while(ContinueExecution);
}

NOIR_STATUS HvInitializeVirtualMachine()
{
	PPROGRAM_HEADER ProgHdr=(PPROGRAM_HEADER)VirtualMemory;
	NOIR_CVM_VIRTUAL_PROCESSOR_OPTIONS VpOpt;
	NOIR_GPR_STATE GprState={0};
	NOIR_CR_STATE CrState={0x80050033,ProgHdr->PagingBase,0x406F8};
	NOIR_SR_STATE SrState;
	NOIR_FG_STATE FgState;
	NOIR_FX_STATE FxState={0};
	SEGMENT_REGISTER DtState[2];
	SEGMENT_REGISTER LtState[2];
	ULONG64 Dr67[2]={0xFFFF0FF0,0x400};
	ULONG64 Rflags=0x2,Rip=ProgHdr->EntryPoint,Xcr0=1,Efer=0xD00;
	NOIR_ADDRESS_MAPPING AddrMap;
	NOIR_STATUS st;
	// Map Virtual Memory
	AddrMap.GPA=0;
	AddrMap.HVA=(ULONG64)VirtualMemory;
	AddrMap.NumberOfPages=VIRTUAL_MEMORY_SIZE>>12;
	AddrMap.Attributes.Value=0;
	AddrMap.Attributes.Present=1;
	AddrMap.Attributes.Write=1;
	AddrMap.Attributes.Execute=1;
	AddrMap.Attributes.User=1;
	AddrMap.Attributes.Caching=NoirMemoryTypeWriteBack;
	st=NoirSetAddressMapping(VmHandle,&AddrMap);
	if(st!=NOIR_SUCCESS)
	{
		printf("Failed to map virtual memory! Status=0x%X\n",st);
		return st;
	}
	// Initialize Virtual Registers
	printf("Entry Point: 0x%p\n",(PVOID)Rip);
	GprState.Rsp=INIT_STACK_POINTER;
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmGeneralPurposeRegister,&GprState,sizeof(GprState));
	printf("Set General-Purpose Register Status=0x%X\n",st);
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmFlagsRegister,&Rflags,sizeof(Rflags));
	printf("Set Flags Register Status=0x%X\n",st);
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmInstructionPointer,&Rip,sizeof(Rip));
	printf("Set Instruction Pointer Status=0x%X\n",st);
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmControlRegister,&CrState,sizeof(CrState));
	printf("Set Control Register Status=0x%X\n",st);
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmDr67Register,Dr67,sizeof(Dr67));
	printf("Set DR6/DR7 Register Status=0x%X\n",st);
	// Segment Registers...
	SrState.Ds.Selector=SrState.Es.Selector=SrState.Ss.Selector=0x18;
	SrState.Cs.Selector=0x10;
	SrState.Ds.Attributes=SrState.Es.Attributes=SrState.Ss.Attributes=0xCF93;
	SrState.Cs.Attributes=0x209B;
	SrState.Cs.Limit=SrState.Ds.Limit=SrState.Es.Limit=SrState.Ss.Limit=0xFFFFFFFF;
	SrState.Cs.Base=SrState.Ds.Base=SrState.Es.Base=SrState.Ss.Base=0;
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmSegmentRegister,&SrState,sizeof(SrState));
	printf("Set Segment Register Status=0x%X\n",st);
	FgState.Fs=FgState.Gs=SrState.Ds;
	FgState.KernelGsBase=0;
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmFsGsRegister,&FgState,sizeof(FgState));
	printf("Set fs/gs Segment Status=0x%X\n",st);
	DtState[0].Limit=0x2F;				// GDTR.Limit
	DtState[0].Base=ProgHdr->GdtBase;	// GDTR.Base
	DtState[1].Limit=0xFFF;				// IDTR.Limit
	DtState[1].Base=ProgHdr->IdtBase;	// IDTR.Base
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmDescriptorTable,&DtState,sizeof(DtState));
	printf("Set GDT/IDT Register Status=0x%X\n",st);
	LtState[0].Selector=0x20;			// TR.Selector
	LtState[0].Attributes=0x8B;			// TR.Attributes
	LtState[0].Limit=0x67;				// TR.Limit
	LtState[0].Base=ProgHdr->TssBase;	// TR.Base
	LtState[1].Selector=0;				// LDTR.Selector
	LtState[1].Attributes=0x2;			// LDTR.Attributes
	LtState[1].Limit=0;					// LDTR.Limit
	LtState[1].Base=0;					// LDTR.Base
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmTrLdtrRegister,&LtState,sizeof(LtState));
	printf("Set Task/LDT Register Status=0x%X\n",st);
	// x87 FPU State
	FxState.Fpu.Fcw=0x40;
	FxState.Fpu.Mxcsr=0x1F80;
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmFxState,&FxState,sizeof(FxState));
	printf("Set x87 FPU State Status=0x%X\n",st);
	// XCR0
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmXcr0Register,&Xcr0,sizeof(Xcr0));
	printf("Set XCR0 Register Status=0x%X\n",st);
	// EFER
	st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmEferRegister,&Efer,sizeof(Efer));
	printf("Set EFER Register Status=0x%X\n",st);
	// vCPU Options: Intercept exceptions
	VpOpt.Value=0;
	VpOpt.InterceptExceptions=1;
	st=NoirSetVirtualProcessorOptions(VmHandle,0,NoirCvmGuestVpOptions,VpOpt.Value);
	printf("Set vCPU Options Status: 0x%X\n",st);
	// Exclude #PF interception
	st=NoirSetVirtualProcessorOptions(VmHandle,0,NoirCvmExceptionBitmap,0xFFFFBFFF);
	printf("Set Exception Bitmap Status: 0x%X\n",st);
	return st;
}

BOOL LoadProgram(IN PSTR ProgramFileName)
{
	HANDLE hFile=CreateFileA(ProgramFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		printf("Failed to load guest program! Error Code: %u\n",GetLastError());
		return FALSE;
	}
	else
	{
		LARGE_INTEGER FileSize;
		BOOL bRet=GetFileSizeEx(hFile,&FileSize);
		if(bRet==FALSE)
			printf("Failed to get file size! Error Code: %u\n",GetLastError());
		else
		{
			DWORD dwRead;
			SetFilePointer(hFile,0,NULL,FILE_BEGIN);
			bRet=ReadFile(hFile,VirtualMemory,FileSize.LowPart,&dwRead,NULL);
			if(bRet==FALSE)printf("Failed to read guest program file! Error Code: %u\n",GetLastError());
		}
		CloseHandle(hFile);
		return bRet;
	}
}

BOOL RunConsole()
{
	STARTUPINFOA SI={0};
	BOOL Result;
	SI.cb=sizeof(SI);
	Result=CreateProcessA(NULL,"putty -serial \\\\.\\pipe\\NoirCvmPioExample",NULL,NULL,FALSE,0,NULL,NULL,&SI,&ConsoleProcInfo);
	if(Result==FALSE)printf("Failed to run PuTTY! Error Code: %u\n",GetLastError());
	return Result;
}

BOOL InitPipe()
{
	BOOL Result=FALSE;
	PipeHandle=CreateNamedPipeA("\\\\.\\pipe\\NoirCvmPioExample",PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,PIPE_TYPE_BYTE,1,1024,1024,0,NULL);
	if(PipeHandle==INVALID_HANDLE_VALUE)
		printf("Failed to create console pipe! Error Code: %u\n",GetLastError());
	else
		Result=TRUE;
	return Result;
}

int main(int argc,char* argv[],char* envp[])
{
	BOOL bRet=NoirInitializeLibrary();
	if(bRet)
	{
		if(InitPipe()==FALSE)return 1;
		if(RunConsole()==FALSE)
			puts("Warning: Failed to run PuTTY! You need to connect to guest console (\\\\.\\pipe\\NoirCvmPioExample) manually!");
		else
			ConnectNamedPipe(PipeHandle,NULL);
		VirtualMemory=VirtualAlloc(NULL,VIRTUAL_MEMORY_SIZE,MEM_COMMIT,PAGE_READWRITE);
		if(VirtualMemory==NULL)
			printf("Failed to allocate virtual memory! Error Code: %u\n",GetLastError());
		else
		{
			if(LoadProgram("program.bin"))
			{
				NOIR_STATUS st=NoirCreateVirtualMachine(&VmHandle);
				if(st!=NOIR_SUCCESS)
					printf("Failed to create virtual machine! Status=0x%X\n",st);
				else
				{
					st=NoirCreateVirtualProcessor(VmHandle,0);
					if(st!=NOIR_SUCCESS)
						printf("Failed to create vCPU! Status=0x%X\n",st);
					else
					{
						st=HvInitializeVirtualMachine();
						if(st==NOIR_SUCCESS)HvRunVirtualMachine();
					}
					NoirDeleteVirtualMachine(VmHandle);
				}
			}
			VirtualFree(VirtualMemory,0,MEM_RELEASE);
		}
		CloseHandle(PipeHandle);
	}
	else
	{
		puts("Failed to initialize library!");
		return 1;
	}
	NoirFinalizeLibrary();
	system("pause");
	return 0;
}