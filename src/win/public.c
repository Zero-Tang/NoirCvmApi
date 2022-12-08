#include <Windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <NoirCvmApi.h>
#include "public.h"

NOIR_STATUS NoirRunVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,OUT PNOIR_CVM_EXIT_CONTEXT ExitContext)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	PVOID OutBuff=_malloca(sizeof(NOIR_CVM_EXIT_CONTEXT)+8);
	if(OutBuff)
	{
		ULONG_PTR InBuff[2];
		InBuff[0]=VirtualMachine;
		InBuff[1]=(ULONG_PTR)VpIndex;
		do
		{
			NoirControlDriver(IOCTL_CvmRunVcpu,InBuff,sizeof(InBuff),OutBuff,sizeof(NOIR_CVM_EXIT_CONTEXT)+8,NULL);
			RtlCopyMemory(ExitContext,(PVOID)((ULONG_PTR)OutBuff+8),sizeof(NOIR_CVM_EXIT_CONTEXT));
			// Re-run the vCPU if the scheduler issued an exit.
			// This guarantees a terminatable process.
		}while(ExitContext->InterceptCode==CvSchedulerExit);
		st=*(PULONG32)OutBuff;
		_freea(OutBuff);
	}
	return st;
}

NOIR_STATUS NoirRescindVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG64 InBuff[2]={VirtualMachine,(ULONG64)VpIndex};
	NoirControlDriver(IOCTL_CvmRescindVcpu,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirEditVirtualProcessorRegister(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN NOIR_CVM_REGISTER_TYPE RegisterType,IN PVOID Buffer,IN ULONG32 BufferSize)
{
	NOIR_STATUS st=NOIR_INSUFFICIENT_RESOURCES;
	// Allocate memory from stack to avoid inter-thread serializations by virtue of heap operations.
	PVOID InBuff=_malloca(BufferSize+sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT));
	if(InBuff)
	{
		PNOIR_VIEW_EDIT_REGISTER_CONTEXT Context=(PNOIR_VIEW_EDIT_REGISTER_CONTEXT)InBuff;
		Context->VirtualMachine=VirtualMachine;
		Context->VpIndex=VpIndex;
		Context->RegisterType=RegisterType;
		RtlCopyMemory(&Context->DummyBuffer,Buffer,BufferSize);
		NoirControlDriver(IOCTL_CvmEditVcpuReg,InBuff,BufferSize+sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT),&st,sizeof(st),NULL);
		_freea(InBuff);
	}
	return st;
}

NOIR_STATUS NoirViewVirtualProcessorRegister(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN NOIR_CVM_REGISTER_TYPE RegisterType,OUT PVOID Buffer,IN ULONG32 BufferSize)
{
	NOIR_STATUS st=NOIR_INSUFFICIENT_RESOURCES;
	// Allocate memory from stack to avoid inter-thread serializations by virtue of heap operations.
	PVOID OutBuff=_malloca(BufferSize+sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT));
	if(OutBuff)
	{
		NOIR_VIEW_EDIT_REGISTER_CONTEXT Context;
		Context.VirtualMachine=VirtualMachine;
		Context.VpIndex=VpIndex;
		Context.RegisterType=RegisterType;
		NoirControlDriver(IOCTL_CvmViewVcpuReg,&Context,sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT),OutBuff,BufferSize+8,NULL);
		st=*(PULONG32)OutBuff;
		RtlCopyMemory(Buffer,(PVOID)((ULONG_PTR)OutBuff+8),BufferSize);
		_freea(OutBuff);
	}
	return st;
}

NOIR_STATUS NoirSetVirtualProcessorOptions(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN NOIR_CVM_VIRTUAL_PROCESSOR_OPTION_TYPE Type,IN ULONG32 Data)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG64 InBuff[3];
	InBuff[0]=VirtualMachine;
	InBuff[1]=(ULONG64)VpIndex;
	*(PULONG32)((ULONG_PTR)&InBuff[2]+0)=Type;
	*(PULONG32)((ULONG_PTR)&InBuff[2]+4)=Data;
	NoirControlDriver(IOCTL_CvmSetVcpuOptions,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirSetEventInjection(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN BOOLEAN Valid,IN BYTE Vector,IN BYTE Type,IN BYTE Priority,IN BOOLEAN ErrorCodeValid,IN ULONG32 ErrorCode)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	NOIR_CVM_EVENT_INJECTION Event={0};
	ULONG64 InBuff[3];
	Event.Vector=Vector;
	Event.Type=Type;
	Event.ErrorCodeValid=ErrorCodeValid;
	if(Type==0)Event.Priority=Priority;
	Event.Reserved=0;
	Event.Valid=Valid;
	Event.ErrorCode=ErrorCode;
	InBuff[0]=VirtualMachine;
	InBuff[1]=(ULONG64)VpIndex;
	InBuff[2]=Event.Value;
	NoirControlDriver(IOCTL_CvmInjectEvent,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirSetAddressMapping(IN CVM_HANDLE VirtualMachine,IN PNOIR_ADDRESS_MAPPING MappingInformation)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG64 InBuff[4];
	BOOL bRet;
	RtlCopyMemory(InBuff,MappingInformation,sizeof(NOIR_ADDRESS_MAPPING));
	InBuff[3]=(ULONG64)VirtualMachine;
	bRet=NoirControlDriver(IOCTL_CvmSetMapping,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirQueryGpaAccessingBitmap(IN CVM_HANDLE VirtualMachine,IN ULONG64 GpaStart,IN ULONG32 NumberOfPages,OUT PVOID Bitmap,IN ULONG32 BitmapSize)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	NOIR_QUERY_ADBITMAP_CONTEXT Context;
	BOOL bRet;
	Context.VirtualMachine=VirtualMachine;
	Context.GpaStart=GpaStart;
	Context.BitmapBuffer=(ULONG64)Bitmap;
	Context.BitmapLength=BitmapSize;
	Context.NumberOfPages=NumberOfPages;
	bRet=NoirControlDriver(IOCTL_CvmQueryGpaAdMap,&Context,sizeof(Context),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirClearGpaAccessingBits(IN CVM_HANDLE VirtualMachine,IN ULONG64 GpaStart,IN ULONG32 NumberOfPages)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	NOIR_QUERY_ADBITMAP_CONTEXT Context;
	BOOL bRet;
	Context.VirtualMachine=VirtualMachine;
	Context.GpaStart=GpaStart;
	Context.NumberOfPages=NumberOfPages;
	bRet=NoirControlDriver(IOCTL_CvmClearGpaAdMap,&Context,sizeof(Context),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirCreateVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG_PTR InBuff[2]={VirtualMachine,VpIndex};
	BOOL bRet=NoirControlDriver(IOCTL_CvmCreateVcpu,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	if(bRet)NoirDebugPrint("vCPU Creation Status=0x%X\n",st);
	return st;
}

NOIR_STATUS NoirDeleteVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG_PTR InBuff[2]={VirtualMachine,VpIndex};
	BOOL bRet=NoirControlDriver(IOCTL_CvmDeleteVcpu,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	if(bRet)NoirDebugPrint("vCPU Deletion Status=0x%X\n",st);
	return st;
}

NOIR_STATUS NoirCreateVirtualMachine(OUT PCVM_HANDLE VirtualMachine)
{
	ULONG_PTR OutBuff[2];
	BOOL bRet=NoirControlDriver(IOCTL_CvmCreateVm,NULL,0,OutBuff,sizeof(OutBuff),NULL);
	if(bRet)
	{
		NOIR_STATUS st=(NOIR_STATUS)OutBuff[0];
		if(st==NOIR_SUCCESS)
			*VirtualMachine=(CVM_HANDLE)OutBuff[1];
		else
			NoirDebugPrint("Failed to create VM! Status=0x%X\n",st);
		return st;
	}
	NoirDebugPrint("Failed to communicate with NoirVisor Driver! Error Code=%u\n",GetLastError());
	return NOIR_UNSUCCESSFUL;
}

NOIR_STATUS NoirDeleteVirtualMachine(IN CVM_HANDLE VirtualMachine)
{
	NOIR_STATUS st;
	NoirControlDriver(IOCTL_CvmDeleteVm,&VirtualMachine,sizeof(VirtualMachine),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirQueryHypervisorStatus(IN NOIR_CVM_HYPERVISOR_STATUS_TYPE StatusType,OUT PVOID Status)
{
	ULONG64 OutBuff[3];
	ULONG64 InBuff=StatusType;
	NoirControlDriver(IOCTL_CvmQueryHvStatus,&InBuff,sizeof(InBuff),OutBuff,sizeof(OutBuff),NULL);
	RtlCopyMemory(Status,&OutBuff[1],16);
	return (NOIR_STATUS)OutBuff[0];
}