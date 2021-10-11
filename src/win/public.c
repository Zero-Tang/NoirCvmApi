#include <Windows.h>
#include <stdlib.h>
#include <malloc.h>
#include "public.h"
#include "ncvmapi.h"

NOIR_STATUS NoirRunVirtualProcessor(IN CVM_HANDLE VmHandle,IN ULONG32 VpIndex,OUT PNOIR_CVM_EXIT_CONTEXT ExitContext)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	PVOID OutBuff=_malloca(sizeof(NOIR_CVM_EXIT_CONTEXT)+8);
	if(OutBuff)
	{
		ULONG_PTR InBuff[2];
		InBuff[0]=VmHandle;
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

NOIR_STATUS NoirEditVirtualProcessorRegister(IN CVM_HANDLE VmHandle,IN ULONG32 VpIndex,IN NOIR_CVM_REGISTER_TYPE RegisterType,IN PVOID Buffer,IN ULONG32 BufferSize)
{
	NOIR_STATUS st=NOIR_INSUFFICIENT_RESOURCES;
	// Allocate memory from stack to avoid inter-thread serializations by virtue of heap operations.
	PVOID InBuff=_malloca(BufferSize+sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT));
	if(InBuff)
	{
		PNOIR_VIEW_EDIT_REGISTER_CONTEXT Context=(PNOIR_VIEW_EDIT_REGISTER_CONTEXT)InBuff;
		Context->VirtualMachine=VmHandle;
		Context->VpIndex=VpIndex;
		Context->RegisterType=RegisterType;
		RtlCopyMemory(&Context->DummyBuffer,Buffer,BufferSize);
		NoirControlDriver(IOCTL_CvmEditVcpuReg,InBuff,BufferSize+sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT),&st,sizeof(st),NULL);
		_freea(InBuff);
	}
	return st;
}

NOIR_STATUS NoirViewVirtualProcessorRegister(IN CVM_HANDLE VmHandle,IN ULONG32 VpIndex,IN NOIR_CVM_REGISTER_TYPE RegisterType,OUT PVOID Buffer,IN ULONG32 BufferSize)
{
	NOIR_STATUS st=NOIR_INSUFFICIENT_RESOURCES;
	// Allocate memory from stack to avoid inter-thread serializations by virtue of heap operations.
	PVOID OutBuff=_malloca(BufferSize+sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT));
	if(OutBuff)
	{
		NOIR_VIEW_EDIT_REGISTER_CONTEXT Context;
		Context.VirtualMachine=VmHandle;
		Context.VpIndex=VpIndex;
		Context.RegisterType=RegisterType;
		NoirControlDriver(IOCTL_CvmViewVcpuReg,&Context,sizeof(NOIR_VIEW_EDIT_REGISTER_CONTEXT),OutBuff,BufferSize+8,NULL);
		st=*(PULONG32)OutBuff;
		RtlCopyMemory(Buffer,(PVOID)((ULONG_PTR)OutBuff+8),BufferSize);
		_freea(OutBuff);
	}
	return st;
}

NOIR_STATUS NoirSetAddressMapping(IN CVM_HANDLE VmHandle,IN PNOIR_ADDRESS_MAPPING MappingInformation)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG64 InBuff[4];
	BOOL bRet;
	RtlCopyMemory(InBuff,MappingInformation,sizeof(NOIR_ADDRESS_MAPPING));
	InBuff[3]=(ULONG64)VmHandle;
	bRet=NoirControlDriver(IOCTL_CvmSetMapping,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	return st;
}

NOIR_STATUS NoirCreateVirtualProcessor(IN CVM_HANDLE VmHandle,IN ULONG32 VpIndex)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG_PTR InBuff[2]={VmHandle,VpIndex};
	BOOL bRet=NoirControlDriver(IOCTL_CvmCreateVcpu,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	if(bRet)NoirDebugPrint("vCPU Creation Status=0x%X\n",st);
	return st;
}

NOIR_STATUS NoirDeleteVirtualProcessor(IN CVM_HANDLE VmHandle,IN ULONG32 VpIndex)
{
	NOIR_STATUS st=NOIR_UNSUCCESSFUL;
	ULONG_PTR InBuff[2]={VmHandle,VpIndex};
	BOOL bRet=NoirControlDriver(IOCTL_CvmDeleteVcpu,InBuff,sizeof(InBuff),&st,sizeof(st),NULL);
	if(bRet)NoirDebugPrint("vCPU Deletion Status=0x%X\n",st);
	return st;
}

NOIR_STATUS NoirCreateVirtualMachine(OUT PCVM_HANDLE VmHandle)
{
	ULONG_PTR OutBuff[2];
	BOOL bRet=NoirControlDriver(IOCTL_CvmCreateVm,NULL,0,OutBuff,sizeof(OutBuff),NULL);
	if(bRet)
	{
		NOIR_STATUS st=(NOIR_STATUS)OutBuff[0];
		if(st==NOIR_SUCCESS)
			*VmHandle=(CVM_HANDLE)OutBuff[1];
		else
			NoirDebugPrint("Failed to create VM! Status=0x%X\n",st);
		return st;
	}
	NoirDebugPrint("Failed to communicate with NoirVisor Driver! Error Code=%u\n",GetLastError());
	return NOIR_UNSUCCESSFUL;
}

NOIR_STATUS NoirDeleteVirtualMachine(IN CVM_HANDLE VmHandle)
{
	NOIR_STATUS st;
	NoirControlDriver(IOCTL_CvmDeleteVm,&VmHandle,sizeof(VmHandle),&st,sizeof(st),NULL);
	return st;
}