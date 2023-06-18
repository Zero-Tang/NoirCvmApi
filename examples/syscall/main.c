#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <NoirCvmApi.h>
#include "main.h"

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
			MapInfo.NumberOfPages=VirtualMemoryPages;
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
				NOIR_CR_STATE CrState={0x80050033,0,0x406F8};
				NOIR_SR_STATE SrState;
				NOIR_FG_STATE FgState;
				NOIR_FX_STATE FxState;
				NOIR_SYSCALL_MSR_STATE ScState={0};
				SEGMENT_REGISTER DtState[2];
				SEGMENT_REGISTER LtState[2];
				ULONG64 Dr67[2]={0xFFFF0FF0,0x400};
				ULONG64 Rflags=0x102,Rip=ProgramHeader->EntryPoint,Xcr0=1,Efer=0xD01;
				GprState.Rsp=VirtualMemorySize-0x10;
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
				SrState.Cs.Selector=0x20;
				SrState.Ds.Attributes=SrState.Es.Attributes=SrState.Ss.Attributes=0xCFF3;
				SrState.Cs.Attributes=0x20FB;
				SrState.Cs.Limit=SrState.Ds.Limit=SrState.Es.Limit=SrState.Ss.Limit=0xFFFFFFFF;
				SrState.Cs.Base=SrState.Ds.Base=SrState.Es.Base=SrState.Ss.Base=0;
				st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmSegmentRegister,&SrState,sizeof(SrState));
				printf("Set Segment Register Status=0x%X\n",st);
				FgState.Fs=FgState.Gs=SrState.Ds;
				FgState.KernelGsBase=0;
				st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmFsGsRegister,&FgState,sizeof(FgState));
				printf("Set fs/gs Segment Status=0x%X\n",st);
				DtState[0].Limit=0x2F;
				DtState[0].Base=ProgramHeader->GdtBase;
				DtState[1].Limit=0xFFF;
				DtState[1].Base=IdtBase;
				st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmDescriptorTable,&DtState,sizeof(DtState));
				printf("Set GDT/IDT Register Status=0x%X\n",st);
				LtState[0].Selector=0x30;				// TR.Selector
				LtState[0].Attributes=0x8B;				// TR.Attributes
				LtState[0].Limit=0x67;					// TR.Limit
				LtState[0].Base=ProgramHeader->TssBase;	// TR.Base
				LtState[1].Selector=0;					// LDTR.Selector
				LtState[1].Attributes=0x2;				// LDTR.Attributes
				LtState[1].Limit=0;						// LDTR.Limit
				LtState[1].Base=0;						// LDTR.Base
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
				// System Call MSRs
				ScState.Star=0x0023001000000000;
				ScState.LStar=ProgramHeader->SystemCallHandler;
				ScState.SfMask=0x4700;
				st=NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmSysCallMsrRegister,&ScState,sizeof(ScState));
				printf("Set System-Call MSR Status=0x%X\n",st);
				// vCPU Options
				VpOpt.Value=0;
				VpOpt.InterceptExceptions=1;
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
	else
		printf("Failed to create VM! Status=0x%X\n",st);
	return st;
}

BOOL InitProgram(IN PCSTR FileName,IN SIZE_T Offset)
{
	BOOL Result=FALSE;
	HANDLE hFile=CreateFileA(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
		printf("Failed to open file (%s)! Error Code: %u\n",FileName,GetLastError());
	else
	{
		LARGE_INTEGER FileSize;
		Result=GetFileSizeEx(hFile,&FileSize);
		Result&=(FileSize.QuadPart+Offset)<=VirtualMemorySize;
		if(Result)
		{
			ULONG dwRead;
			SetFilePointer(hFile,0,NULL,FILE_BEGIN);
			ReadFile(hFile,(PVOID)((ULONG_PTR)VirtualMemory+Offset),FileSize.LowPart,&dwRead,NULL);
		}
		else
		{
			if(FileSize.QuadPart+Offset<=VirtualMemorySize)
				printf("File Size (%llu bytes) exceeds 2MiB boundary!\n",FileSize.QuadPart);
			else
				printf("Failed to query file size! Error Code: %u\n",GetLastError());
		}
		CloseHandle(hFile);
	}
	return Result;
}

NOIR_STATUS PrintGprState(OUT PNOIR_GPR_STATE GprState)
{
	PULONG64 GprStateArray=(PULONG64)GprState;
	NOIR_STATUS st=NoirViewVirtualProcessorRegister(VmHandle,0,NoirCvmGeneralPurposeRegister,GprState,sizeof(NOIR_GPR_STATE));
	if(st==NOIR_SUCCESS)
	{
		for(ULONG i=0;i<16;i++)
			printf("%s\t0x%016llX%c",GprNames[i],GprStateArray[i],(i+1)&3?'\t':'\n');
	}
	return st;
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
				case CvHypercall:
				{
					NOIR_GPR_STATE GprState;
					printf("Hypercall is intercepted at Rip=0x%llX!\n",ExitContext.Rip);
					PrintGprState(&GprState);
					if(GprState.Rax==0)
						NoirEditVirtualProcessorRegister(VmHandle,0,NoirCvmInstructionPointer,&ExitContext.NextRip,sizeof(ULONG64));
					else if (GprState.Rax==1)
						ContinueExecution=FALSE;
					else
						printf("Unknown rax code (%llu) for Hypercall!\n",GprState.Rax);
					break;
				}
				case CvException:
				{
					switch(ExitContext.Exception.Vector)
					{
						case CvDebugFaultOrTrap:
						{
							NOIR_GPR_STATE GprState;
							printf("Debug Exception is intercepted at Rip=0x%llX!\n",ExitContext.Rip);
							PrintGprState(&GprState);
							break;
						}
						default:
						{
							printf("Unexpected exception vector: %u!\n",ExitContext.Exception.Vector);
							ContinueExecution=FALSE;
							break;
						}
					}
					break;
				}
				default:
				{
					printf("Unknown interception code: %u!\n",ExitContext.InterceptCode);
					ContinueExecution=FALSE;
					break;
				}
			}
		}
	}
}

int main(int argc,char* argv[],char* envp[])
{
	if(NoirInitializeLibrary()==FALSE)
	{
		puts("Failed to initialize NoirCvmApi Library!");
		return 1;
	}
	VirtualMemory=PageAlloc(VirtualMemorySize);
	if(VirtualMemory==NULL)
	{
		printf("Failed to allocate memory for guest! Error Code: %u\n",GetLastError());
		return 2;
	}
	if(InitProgram("paging.bin",0x0000)==FALSE)return 3;
	if(InitProgram("program.bin",0x4000)==FALSE)return 4;
	ProgramHeader=(PPROGRAM_HEADER)((ULONG_PTR)VirtualMemory+0x4000);
	if(InitVM()!=NOIR_SUCCESS)return 5;
	puts("VM is initialized successfully! Now running VM...");
	RunVM();
	system("pause");
	NoirDeleteVirtualMachine(VmHandle);
	PageFree(VirtualMemory);
	return 0;
}