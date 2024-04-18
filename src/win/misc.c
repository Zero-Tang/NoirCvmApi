#include <Windows.h>
#include <strsafe.h>
#include <stdarg.h>
#include "misc.h"

void __cdecl NoirDebugPrint(IN PCSTR Format,...)
{
	CHAR Buffer[512];
	PSTR PrintStart=NULL;
	SIZE_T PrintRemainder=0;
	va_list ArgList;
	StringCbCopyExA(Buffer,sizeof(Buffer),"[NoirVisor CVM API] ",&PrintStart,&PrintRemainder,0);
	va_start(ArgList,Format);
	StringCbVPrintfA(PrintStart,PrintRemainder,Format,ArgList);
	va_end(ArgList);
	OutputDebugStringA(Buffer);
}

PVOID MemAlloc(IN SIZE_T Length)
{
	return HeapAlloc(ProcessHeap,HEAP_ZERO_MEMORY,Length);
}

BOOL MemFree(IN PVOID Memory)
{
	return HeapFree(ProcessHeap,0,Memory);
}

PVOID PageAlloc(IN SIZE_T Length)
{
	PVOID p=VirtualAlloc(NULL,Length,MEM_COMMIT,PAGE_READWRITE);
	if(p)RtlZeroMemory(p,Length);
	return p;
}

BOOL PageFree(IN PVOID Memory)
{
	return VirtualFree(Memory,0,MEM_RELEASE);
}