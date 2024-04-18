#include <Windows.h>
#include "drv_comm.h"

BOOL NoirControlDriver(IN ULONG IoControlCode,IN PVOID InputBuffer,IN ULONG InputSize,OUT PVOID OutputBuffer,IN ULONG OutputSize,OUT PULONG ReturnLength OPTIONAL)
{
	ULONG BytesReturned=0;
	BOOL bRet=DeviceIoControl(NoirVisorDriverHandle,IoControlCode,InputBuffer,InputSize,OutputBuffer,OutputSize,&BytesReturned,NULL);
	if(ReturnLength)*ReturnLength=BytesReturned;
	return bRet;
}

BOOL NoirInitializeLibrary()
{
	NoirVisorDriverHandle=CreateFileW(NV_DEVICE_NAME,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(NoirVisorDriverHandle!=INVALID_HANDLE_VALUE)
	{
		ProcessHeap=GetProcessHeap();
		CurrentProcess=GetCurrentProcess();
		return TRUE;
	}
	else
	{
		DWORD ErrCode=GetLastError();
		NoirDebugPrint("Failed to connect to NoirVisor driver! Error Code=%u!\n",ErrCode);
		if(ErrCode==ERROR_FILE_NOT_FOUND)NoirDebugPrint("Perhaps, you forgot to load NoirVisor Driver!\n");
		return FALSE;
	}
}

void NoirFinalizeLibrary()
{
	if(NoirVisorDriverHandle!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(NoirVisorDriverHandle);
		NoirVisorDriverHandle=INVALID_HANDLE_VALUE;
	}
}