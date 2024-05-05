#include <Windows.h>

// Definitions of I/O Control Codes of NoirVisor Driver CVM Interface.
#define CTL_CODE_GEN(i)		CTL_CODE(FILE_DEVICE_UNKNOWN,i,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_CvmCreateVm		CTL_CODE_GEN(0x880)
#define IOCTL_CvmDeleteVm		CTL_CODE_GEN(0x881)
#define IOCTL_CvmSetMapping		CTL_CODE_GEN(0x882)
#define IOCTL_CvmQueryGpaAdMap  CTL_CODE_GEN(0x883)
#define IOCTL_CvmClearGpaAdMap  CTL_CODE_GEN(0x884)
#define IOCTL_CvmCreateVmEx     CTL_CODE_GEN(0x885)
#define IOCTL_CvmQueryHvStatus	CTL_CODE_GEN(0x88F)
#define IOCTL_CvmCreateVcpu		CTL_CODE_GEN(0x890)
#define IOCTL_CvmDeleteVcpu		CTL_CODE_GEN(0x891)
#define IOCTL_CvmRunVcpu		CTL_CODE_GEN(0x892)
#define IOCTL_CvmViewVcpuReg	CTL_CODE_GEN(0x893)
#define IOCTL_CvmEditVcpuReg	CTL_CODE_GEN(0x894)
#define IOCTL_CvmRescindVcpu	CTL_CODE_GEN(0x895)
#define IOCTL_CvmInjectEvent	CTL_CODE_GEN(0x896)
#define IOCTL_CvmSetVcpuOptions	CTL_CODE_GEN(0x897)
#define IOCTL_CvmQueryVcpuStats	CTL_CODE_GEN(0x898)
#define IOCTL_CvmViewVcpuReg2	CTL_CODE_GEN(0x899)
#define IOCTL_CvmEditVcpuReg2	CTL_CODE_GEN(0x89A)

typedef struct _NOIR_VIEW_EDIT_REGISTER_CONTEXT
{
	CVM_HANDLE VirtualMachine;
	ULONG32 VpIndex;
	NOIR_CVM_REGISTER_TYPE RegisterType;
	PVOID DummyBuffer;
}NOIR_VIEW_EDIT_REGISTER_CONTEXT,*PNOIR_VIEW_EDIT_REGISTER_CONTEXT;

typedef struct _NOIR_VIEW_EDIT_REGISTER_CONTEXT2
{
	CVM_HANDLE VirtualMachine;
	ULONG32 VpIndex;
	ULONG32 RegCount;
	ULONG32 RegSize;
	PNOIR_CVM_REGISTER_NAME RegNames;
	PVOID Buffer;
	NOIR_STATUS *Status;
}NOIR_VIEW_EDIT_REGISTER_CONTEXT2,*PNOIR_VIEW_EDIT_REGISTER_CONTEXT2;

typedef struct _NOIR_QUERY_ADBITMAP_CONTEXT
{
	CVM_HANDLE VirtualMachine;
	ULONG64 GpaStart;
	ULONG64 BitmapBuffer;
	ULONG32 BitmapLength;
	ULONG32 NumberOfPages;
}NOIR_QUERY_ADBITMAP_CONTEXT,*PNOIR_QUERY_ADBITMAP_CONTEXT;

BOOL NoirControlDriver(IN ULONG IoControlCode,IN PVOID InputBuffer,IN ULONG InputSize,OUT PVOID OutputBuffer,IN ULONG OutputSize,OUT PULONG ReturnLength OPTIONAL);
void __cdecl NoirDebugPrint(IN PCSTR Format,...);