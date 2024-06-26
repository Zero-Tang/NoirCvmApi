#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <NoirCvmApi.h>


#define PagingBase					0x00000000
#define GdtBase						0x00003000
#define TssBase						0x00003030
#define InitStackOffset				0x00003FF0
#define IdtBase						0x00004000
#define ProgramStartOffset			0x00005000
#define InsecureMemoryOffset		0x00100000
#define VirtualMemorySize			0x00200000
#define VirtualMemoryPages			(VirtualMemorySize>>12)

#define MmioGpaVersion				0x00200000
#define MmioGpaConsole				0x00200004
#define MmioGpaPower				0x00200008

NOIR_STATUS HvIoPortCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_IO_ACCESS_INFO IoAccess);
NOIR_STATUS HvMemoryCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess);
NOIR_STATUS HvViewRegisterCallback(IN OUT PVOID Context,IN PNOIR_CVM_REGISTER_NAME RegisterNames,IN ULONG32 RegisterCount,IN ULONG32 RegisterSize,OUT PVOID RegisterValues);
NOIR_STATUS HvEditRegisterCallback(IN OUT PVOID Context,IN PNOIR_CVM_REGISTER_NAME RegisterNames,IN ULONG32 RegisterCount,IN ULONG32 RegisterSize,IN PVOID RegisterValues);
NOIR_STATUS HvTranslateGvaPageCallback(IN OUT PVOID Context,IN ULONG64 GvaPage,IN NOIR_TRANSLATE_GVA_FLAGS TranslationFlags,OUT PNOIR_TRANSLATE_GVA_RESULT TranslationResult,OUT PULONG64 GpaPage);
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

HANDLE PipeHandle=INVALID_HANDLE_VALUE;
CVM_HANDLE VmHandle=0;
PVOID VirtualMemory=NULL;
PROCESS_INFORMATION ConsoleProcInfo;

NOIR_STATUS InitVM()
{
	NOIR_STATUS st=NoirCreateVirtualMachine(&VmHandle);
	if(st==NOIR_SUCCESS)
	{
		st=NoirCreateVirtualProcessor(VmHandle,0);
		if(st==NOIR_SUCCESS)
		{
			NOIR_ADDRESS_MAPPING MapInfo;
			MapInfo.GPA=0;
			MapInfo.HVA=(ULONG64)VirtualMemory;
			MapInfo.NumberOfPages=VirtualMemorySize>>12;
			MapInfo.Attributes.Value=0;
			MapInfo.Attributes.Present=TRUE;
			MapInfo.Attributes.Write=TRUE;
			MapInfo.Attributes.Execute=TRUE;
			MapInfo.Attributes.User=TRUE;
			MapInfo.Attributes.Caching=NoirMemoryTypeWriteBack;
			st=NoirSetAddressMapping(VmHandle,&MapInfo);
			if(st==NOIR_SUCCESS)
			{
				NOIR_CVM_VIRTUAL_PROCESSOR_OPTIONS VpOpt;
				// Initialize the processor state.
				NOIR_GPR_STATE GprState={0};
				NOIR_CR_STATE CrState={0x80050033,PagingBase,0x406F8};
				NOIR_SR_STATE SrState;
				NOIR_FG_STATE FgState;
				NOIR_FX_STATE FxState={0};
				SEGMENT_REGISTER DtState[2];
				SEGMENT_REGISTER LtState[2];
				ULONG64 Dr67[2]={0xFFFF0FF0,0x400};
				ULONG64 Rflags=0x2,Rip=ProgramStartOffset,Xcr0=1,Efer=0xD00;
				printf("Entry Point: 0x%p\n",(PVOID)Rip);
				GprState.Rsp=InitStackOffset;
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
				DtState[0].Limit=0x2F;		// GDTR.Limit
				DtState[0].Base=GdtBase;	// GDTR.Base
				DtState[1].Limit=0xFFF;		// IDTR.Limit
				DtState[1].Base=IdtBase;	// IDTR.Base
				st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmDescriptorTable,&DtState,sizeof(DtState));
				printf("Set GDT/IDT Register Status=0x%X\n",st);
				LtState[0].Selector=0x20;	// TR.Selector
				LtState[0].Attributes=0x8B;	// TR.Attributes
				LtState[0].Limit=0x67;		// TR.Limit
				LtState[0].Base=TssBase;	// TR.Base
				LtState[1].Selector=0;		// LDTR.Selector
				LtState[1].Attributes=0x2;	// LDTR.Attributes
				LtState[1].Limit=0;			// LDTR.Limit
				LtState[1].Base=0;			// LDTR.Base
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
				// vCPU Options
				VpOpt.Value=0;
				VpOpt.InterceptExceptions=1;
				VpOpt.DecodeMemoryAccessInstruction=1;
				st=NoirSetVirtualProcessorOptions(VmHandle,0,NoirCvmGuestVpOptions,VpOpt.Value);
				printf("Set vCPU Options Status: 0x%X\n",st);
				st=NoirSetVirtualProcessorOptions(VmHandle,0,NoirCvmExceptionBitmap,0xFFFFFFFF);
				printf("Set Exception Bitmap Status: 0x%X\n",st);
			}
			else
			{
				printf("Failed to set mapping! Status=0x%X\n",st);
				NoirDeleteVirtualMachine(VmHandle);
			}
		}
		else
		{
			printf("Failed to create vCPU! Status=0x%X\n",st);
			NoirDeleteVirtualMachine(VmHandle);
		}
	}
	return st;
}

NOIR_STATUS HvIoPortCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_IO_ACCESS_INFO IoAccess)
{
	puts("Port I/O is not implemented in this example!");
	return NOIR_NOT_IMPLEMENTED;
}

NOIR_STATUS HvMemoryCallback(IN OUT PVOID Context,IN OUT PNOIR_EMULATOR_MEMORY_ACCESS_INFO MemoryAccess)
{
	// Note that this callback does not necessarily mean Memory-Mapped I/O.
	if(MemoryAccess->Gpa+MemoryAccess->AccessSize<VirtualMemorySize)
	{
		// Regular Memory Access.
		PVOID Hva=(PVOID)((ULONG64)VirtualMemory+MemoryAccess->Gpa);
		if(MemoryAccess->Direction)
			RtlCopyMemory(Hva,MemoryAccess->Data,MemoryAccess->AccessSize);
		else
			RtlCopyMemory(MemoryAccess->Data,Hva,MemoryAccess->AccessSize);
		return NOIR_SUCCESS;
	}
	else
	{
		PBOOLEAN ContinueExecution=(PBOOLEAN)Context;
		// Memory-Mapped I/O.
		switch(MemoryAccess->Gpa)
		{
			case MmioGpaVersion:
			{
				if(MemoryAccess->Direction)
				{
					puts("You can't write to a version register!");
				}
				else
				{
					*(PULONG32)MemoryAccess->Data=0x12345678;
				}
				break;
			}
			case MmioGpaConsole:
			{
				if(MemoryAccess->Direction)
				{
					DWORD dwWrite;
					WriteFile(PipeHandle,MemoryAccess->Data,1,&dwWrite,NULL);
				}
				else
				{
					puts("Reading from console is currently unsupported!");
				}
				break;
			}
			case MmioGpaPower:
			{
				if(MemoryAccess->Direction)
				{
					if(*(PULONG32)MemoryAccess->Data==0xAA55)
					{
						puts("Shutdown is requested!");
						*ContinueExecution=FALSE;
					}
				}
				break;
			}
			default:
			{
				printf("Unknown MMIO Address! GPA=0x%llX\n",MemoryAccess->Gpa);
				break;
			}
		}
	}
	return NOIR_SUCCESS;
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
	if(GvaPage<VirtualMemorySize)
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

void RunVM()
{
	NOIR_CVM_EXIT_CONTEXT ExitContext;
	BOOL ContinueExecution=TRUE;
	while(ContinueExecution)
	{
		NOIR_STATUS st=NoirRunVirtualProcessor(VmHandle,0,&ExitContext);
		if(st==NOIR_SUCCESS)
		{
			switch(ExitContext.InterceptCode)
			{
				case CvInvalidState:
				{
					puts("The vCPU state is invalid!");
					ContinueExecution=FALSE;
					break;
				}
				case CvShutdownCondition:
				{
					puts("The vCPU triggers a shutdown condition!");
					ContinueExecution=FALSE;
					break;
				}
				case CvMemoryAccess:
				{
					NOIR_EMULATION_STATUS EmuSt;
					printf("Memory-Access was intercepted at GPA=0x%llX! Rip=0x%llX!\n",ExitContext.MemoryAccess.Gpa,ExitContext.Rip);
					printf("Instruction Bytes (%llu): ",ExitContext.NextRip-ExitContext.Rip);
					for(ULONG64 i=0;i<ExitContext.NextRip-ExitContext.Rip;i++)printf("%02X ",ExitContext.MemoryAccess.InstructionBytes[i]);
					putc('\n',stdout);
					// MMIO.
					st=NoirTryMmioEmulation(&EmulatorCallbacks,&ContinueExecution,&ExitContext,&EmuSt);
					if(st!=NOIR_SUCCESS)
					{
						printf("Failed to emulate MMIO instruction! Return-Status: 0x%llX\n",EmuSt.Value);
						ContinueExecution=FALSE;
					}
					break;
				}
				case CvException:
				{
					printf("Exception Vector %u is intercepted at rip=0x%llX!\n",ExitContext.Exception.Vector,ExitContext.Rip);
					printf("Error Code is 0x%X\n",ExitContext.Exception.ErrorCode);
					if(ExitContext.Exception.Vector==14)printf("#PF Linear Address: 0x%p\n",(PVOID)ExitContext.Exception.PageFaultAddress);
					ContinueExecution=FALSE;
					break;
				}
				default:
				{
					printf("Unknown interception code: 0x%u!\n",ExitContext.InterceptCode);
					break;
				}
			}
		}
	}
}

BOOL InitGuestProgram(IN PSTR FilePath)
{
	BOOL Result=FALSE;
	HANDLE hFile=CreateFileA(FilePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
		printf("Failed to open file! Error Code: %u\n",GetLastError());
	else
	{
		LARGE_INTEGER FileSize;
		Result=GetFileSizeEx(hFile,&FileSize);
		Result&=FileSize.QuadPart<=VirtualMemorySize;
		if(Result)
		{
			VirtualMemory=PageAlloc(VirtualMemorySize);
			if(VirtualMemory==NULL)
				printf("Failed to allocate guest memory! Error Code: %u\n",GetLastError());
			else
			{
				ULONG dwRead;
				SetFilePointer(hFile,0,NULL,FILE_BEGIN);
				ReadFile(hFile,VirtualMemory,FileSize.LowPart,&dwRead,NULL);
			}
		}
		else
		{
			if(FileSize.QuadPart<=VirtualMemorySize)
				printf("File Size (%llu bytes) exceeds 2MiB!\n",FileSize.QuadPart);
			else
				printf("Failed to query file size! Error Code: %u\n",GetLastError());
		}
		CloseHandle(hFile);
	}
	return Result;
}

BOOL RunConsole()
{
	STARTUPINFOA SI={0};
	SI.cb=sizeof(SI);
	BOOL Result=CreateProcessA(NULL,"putty -serial \\\\.\\pipe\\NoirCvmMmioExample",NULL,NULL,FALSE,0,NULL,NULL,&SI,&ConsoleProcInfo);
	if(Result==FALSE)printf("Failed to run PuTTY! Error Code: %u\n",GetLastError());
	return Result;
}

BOOL InitPipe()
{
	BOOL Result=FALSE;
	PipeHandle=CreateNamedPipeA("\\\\.\\pipe\\NoirCvmMmioExample",PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,PIPE_TYPE_BYTE,1,1024,1024,0,NULL);
	if(PipeHandle==INVALID_HANDLE_VALUE)
		printf("Failed to create console pipe! Error Code: %u\n",GetLastError());
	else
		Result=TRUE;
	return Result;
}

int main(int argc,char* argv[],char* envp[])
{
	if(NoirInitializeLibrary()==FALSE)
	{
		puts("Failed to initialize NoirCvmApi Library!");
		return 4;
	}
	if(InitPipe()==FALSE)return 1;
	if(RunConsole()==FALSE)
		printf("Warning: Failed to run PuTTY! You need to connect to guest console (\\\\.\\pipe\\NoirCvmMmioExample) manually!\n");
	else
		ConnectNamedPipe(PipeHandle,NULL);
	if(InitGuestProgram("program.bin")==FALSE)return 2;
	if(InitVM()!=NOIR_SUCCESS)return 3;
	puts("VM is initialized successfully! Now running VM...");
	RunVM();
	system("pause");
	NoirDeleteVirtualMachine(VmHandle);
	PageFree(VirtualMemory);
	CloseHandle(PipeHandle);
	TerminateProcess(ConsoleProcInfo.hProcess,0);
	return 0;
}