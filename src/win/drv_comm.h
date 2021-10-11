#include <Windows.h>

#define NV_DEVICE_NAME			L"\\\\.\\NoirVisor"

void __cdecl NoirDebugPrint(IN PCSTR Format,...);

HANDLE NoirVisorDriverHandle=INVALID_HANDLE_VALUE;
HANDLE ProcessHeap=NULL;
HANDLE CurrentProcess=NULL;
SIZE_T MinWSet=0;
SIZE_T MaxWSet=0;