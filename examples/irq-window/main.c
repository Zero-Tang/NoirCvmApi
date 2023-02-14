#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <strsafe.h>
#include <NoirCvmApi.h>
#include "main.h"

void Finalize()
{
	NoirDeleteVirtualMachine(VmHandle);
	PageFree(VirtualMemory);
	NoirFinalizeLibrary();
}

NOIR_STATUS InitVP()
{
	NOIR_STATUS st;
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
	ULONG64 Rflags=0x2,Rip=EntryPointGva,Xcr0=1,Efer=0xD00;
	printf("Entry Point: 0x%p\n",(PVOID)Rip);
	GprState.Rsp=StackTop;
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
	VpOpt.InterceptInterruptWindow=1;
	VpOpt.InterceptExceptions=1;
	st=NoirSetVirtualProcessorOptions(VmHandle,0,NoirCvmGuestVpOptions,VpOpt.Value);
	printf("Set vCPU Options Status: 0x%X\n",st);
	st=NoirSetVirtualProcessorOptions(VmHandle,0,NoirCvmExceptionBitmap,0xFFFFFFFF);
	printf("Set Exception Bitmap Status: 0x%X\n",st);
	return st;
}

BOOL LoadProgram(IN PSTR MiscFilePath,IN PSTR ProgramFilePath)
{
	HANDLE hFile=CreateFileA(MiscFilePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	BOOL Result;
	if(hFile==INVALID_HANDLE_VALUE)
	{
		printf("Failed to open file! Error Code: %u\n",GetLastError());
		return FALSE;
	}
	else
	{
		LARGE_INTEGER FileSize;
		Result=GetFileSizeEx(hFile,&FileSize);
		Result&=FileSize.QuadPart<=ProgramBase;
		if(Result)
		{
			ULONG dwRead;
			SetFilePointer(hFile,0,NULL,FILE_BEGIN);
			ReadFile(hFile,VirtualMemory,FileSize.LowPart,&dwRead,NULL);
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
	hFile=CreateFileA(ProgramFilePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		printf("Failed to open file! Error Code: %u\n",GetLastError());
		return FALSE;
	}
	else
	{
		// FIXME: This loader does not have relocation.
		ULONG dwRead;
		PIMAGE_DOS_HEADER DosHead=(PIMAGE_DOS_HEADER)((ULONG_PTR)VirtualMemory+ProgramBase);
		SetFilePointer(hFile,0,NULL,FILE_BEGIN);
		ReadFile(hFile,DosHead,sizeof(IMAGE_DOS_HEADER),&dwRead,NULL);
		if(DosHead->e_magic==IMAGE_DOS_SIGNATURE)
		{
			PIMAGE_NT_HEADERS NtHead=(PIMAGE_NT_HEADERS)((ULONG_PTR)DosHead+DosHead->e_lfanew);
			SetFilePointer(hFile,DosHead->e_lfanew,NULL,FILE_BEGIN);
			ReadFile(hFile,NtHead,sizeof(IMAGE_NT_HEADERS),&dwRead,NULL);
			if(NtHead)
			{
				PIMAGE_SECTION_HEADER SectionHeaders=(PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHead+sizeof(IMAGE_NT_HEADERS));
				ReadFile(hFile,SectionHeaders,sizeof(IMAGE_SECTION_HEADER)*NtHead->FileHeader.NumberOfSections,&dwRead,NULL);
				for(USHORT i=0;i<NtHead->FileHeader.NumberOfSections;i++)
				{
					PVOID Section=(PVOID)((ULONG_PTR)DosHead+SectionHeaders[i].VirtualAddress);
					SetFilePointer(hFile,SectionHeaders[i].PointerToRawData,NULL,FILE_BEGIN);
					ReadFile(hFile,Section,SectionHeaders[i].SizeOfRawData,&dwRead,NULL);
				}
				EntryPointGva=ProgramBase+NtHead->OptionalHeader.AddressOfEntryPoint;
			}
		}
		CloseHandle(hFile);
	}
	return Result;
}

BOOL Initialize()
{
	BOOL Result=NoirInitializeLibrary();
	if(Result)
	{
		NOIR_STATUS st;
		StdInHandle=GetStdHandle(STD_INPUT_HANDLE);
		StdOutHandle=GetStdHandle(STD_OUTPUT_HANDLE);
		StdErrHandle=GetStdHandle(STD_ERROR_HANDLE);
		st=NoirCreateVirtualMachine(&VmHandle);
		if(st!=NOIR_SUCCESS)
			printf("Failed to create virtual machine! Status=0x%X\n",st);
		else
		{
			st=NoirCreateVirtualProcessor(VmHandle,0);
			if(st==NOIR_SUCCESS)
			{
				VirtualMemory=PageAlloc(VirtualMemorySize);
				if(VirtualMemory)
				{
					NOIR_ADDRESS_MAPPING MapInfo;
					MapInfo.GPA=(ULONG64)0;
					MapInfo.HVA=(ULONG64)VirtualMemory;
					MapInfo.NumberOfPages=VirtualPages;
					MapInfo.Attributes.Value=0;
					MapInfo.Attributes.Present=1;
					MapInfo.Attributes.Write=1;
					MapInfo.Attributes.Execute=1;
					MapInfo.Attributes.User=1;
					MapInfo.Attributes.Caching=NoirMemoryTypeWriteBack;
					st=NoirSetAddressMapping(VmHandle,&MapInfo);
					if(st!=NOIR_SUCCESS)printf("Failed to set address mapping! Status=0x%X\n",st);
					Result=LoadProgram("misc.bin","IrqWindowGuest.exe");
					if(!Result)printf("Failed to load program!\n");
					st=InitVP();
					if(st!=NOIR_SUCCESS)printf("Failed to initialize vCPU! Status=0x%X\n",st);
				}
				else
				{
					printf("Failed to allocate virtual memory! Error Code=%u\n",GetLastError());
					NoirDeleteVirtualMachine(VmHandle);
				}
			}
			else
			{
				printf("Failed to create virtual processor! Status=0x%X\n",st);
				NoirDeleteVirtualMachine(VmHandle);
			}
		}
		Result=st==NOIR_SUCCESS;
	}
	return Result;
}

NOIR_STATUS RunVM()
{
	NOIR_CVM_EXIT_CONTEXT ExitContext;
	BOOLEAN ContinueExecution=TRUE,Interrupt2=TRUE;
	NOIR_STATUS st=NOIR_SUCCESS;
	// At start, inject an interrupt.
	NoirSetEventInjection(VmHandle,0,TRUE,0x30,NoirEventTypeExternalInterrupt,15,FALSE,0);
	while(ContinueExecution)
	{
		st=NoirRunVirtualProcessor(VmHandle,0,&ExitContext);
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
					puts("Unexpected Memory Access Interception!");
					printf("GPA=0x%llX\n",ExitContext.MemoryAccess.Gpa);
					ContinueExecution=FALSE;
					break;
				}
				case CvHltInstruction:
				{
					printf("Program is halted at rip=0x%llX!\n",ExitContext.Rip);
					ContinueExecution=FALSE;
					break;
				}
				case CvIoInstruction:
				{
					if(ExitContext.Io.Access.String && ExitContext.Io.Access.Repeat && ExitContext.Io.Access.IoType==0 && ExitContext.Io.Port==1)
					{
						printf("[Guest StdOut] %.*s",(ULONG32)ExitContext.Io.Rcx,(PSTR)((ULONG_PTR)VirtualMemory+ExitContext.Io.Rsi));
					}
					else
					{
						printf("Unexpected I/O (Type=%u, String=%u, Repeat=%u, Port=0x%04X)!\n",ExitContext.Io.Access.String,ExitContext.Io.Access.Repeat,ExitContext.Io.Access.IoType==0,ExitContext.Io.Port);
						ContinueExecution=FALSE;
					}
					NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmInstructionPointer,&ExitContext.NextRip,sizeof(ExitContext.NextRip));
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
				case CvInterruptWindow:
				{
					printf("Interrupt-Window is intercepted at rip=0x%llX!\n",ExitContext.Rip);
					if(Interrupt2)NoirSetEventInjection(VmHandle,0,TRUE,0x40,NoirEventTypeExternalInterrupt,15,FALSE,0);
					Interrupt2=FALSE;
					break;
				}
				default:
				{
					printf("Unknown interception code: 0x%u!\n",ExitContext.InterceptCode);
					ContinueExecution=FALSE;
					break;
				}
			}
		}
	}
	return st;
}

int main(int argc,char* argv[],char* envp[])
{
	BOOL Result=Initialize();
	if(Result==FALSE)return 1;
	RunVM();
	system("pause");
	Finalize();
}