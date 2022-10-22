// Definitions of Status Codes of NoirVisor.
#define NOIR_SUCCESS					0
#define NOIR_UNSUCCESSFUL				0xC0000000
#define NOIR_INSUFFICIENT_RESOURCES		0xC0000001
#define NOIR_NOT_IMPLEMENTED			0xC0000002
#define NOIR_UNKNOWN_PROCESSOR			0xC0000003
#define NOIR_INVALID_PARAMETER			0xC0000004
#define NOIR_HYPERVISION_ABSENT			0xC0000005
#define NOIR_VCPU_ALREADY_CREATED		0xC0000006
#define NOIR_BUFFER_TOO_SMALL			0xC0000007
#define NOIR_VCPU_NOT_EXIST				0xC0000008
#define NOIR_USER_PAGE_VIOLATION		0xC0000009

typedef ULONG32 NOIR_STATUS;

// Layered Hypervisor Functions
typedef ULONG_PTR CVM_HANDLE;
typedef PULONG_PTR PCVM_HANDLE;

typedef union _NOIR_CVM_EVENT_INJECTION
{
	struct
	{
		ULONG64 Vector:8;
		ULONG64 Type:3;
		ULONG64 ErrorCodeValid:1;
		ULONG64 Reserved:15;
		ULONG64 Priority:4;
		ULONG64 Valid:1;
		ULONG64 ErrorCode:32;
	};
	ULONG64 Value;
}NOIR_CVM_EVENT_INJECTION,*PNOIR_CVM_EVENT_INJECTION;

typedef enum _NOIR_CVM_VIRTUAL_PROCESSOR_OPTION_TYPE
{
	NoirCvmGuestVpOptions,
	NoirCvmExceptionBitmap,
	NoirCvmSchedulingPriority
}NOIR_CVM_VIRTUAL_PROCESSOR_OPTION_TYPE;*PNOIR_CVM_VIRTUAL_PROCESSOR_OPTION_TYPE;

typedef struct _NOIR_GPR_STATE
{
	ULONG_PTR Rax;
	ULONG_PTR Rcx;
	ULONG_PTR Rdx;
	ULONG_PTR Rbx;
	ULONG_PTR Rsp;
	ULONG_PTR Rbp;
	ULONG_PTR Rsi;
	ULONG_PTR Rdi;
#if defined(_WIN64)
	ULONG64 R8;
	ULONG64 R9;
	ULONG64 R10;
	ULONG64 R11;
	ULONG64 R12;
	ULONG64 R13;
	ULONG64 R14;
	ULONG64 R15;
#endif
}NOIR_GPR_STATE,*PNOIR_GPR_STATE;

typedef struct _NOIR_CR_STATE
{
	ULONG64 Cr0;
	ULONG64 Cr3;
	ULONG64 Cr4;
}NOIR_CR_STATE,*PNOIR_CR_STATE;

typedef union _R128
{
	float f[4];
	double d[2];
}R128;

typedef struct _NOIR_XMM_STATE
{
	R128 Xmm0;
	R128 Xmm1;
	R128 Xmm2;
	R128 Xmm3;
	R128 Xmm4;
	R128 Xmm5;
	R128 Xmm6;
	R128 Xmm7;
#if defined(_WIN64)
	R128 Xmm8;
	R128 Xmm9;
	R128 Xmm10;
	R128 Xmm11;
	R128 Xmm12;
	R128 Xmm13;
	R128 Xmm14;
	R128 Xmm15;
#endif
}NOIR_XMM_STATE,*NOIR_XMM_STATE_P;

// 512-byte region of fxsave instruction.
// Including FPU, x87, XMM.
typedef struct _NOIR_FX_STATE
{
	struct
	{
		USHORT Fcw;
		USHORT Fsw;
		BYTE Ftw;
		BYTE Reserved;
		USHORT Fop;
		ULONG32 Fip;
		USHORT Fcs;
		USHORT Reserved1;
#if defined(_WIN64)
		ULONG64 Fdp;
#else
		ULONG32 Fdp;
		USHORT Fds;
		USHORT Reserved2;
#endif
		ULONG32 Mxcsr;
		ULONG32 MxcsrMask;
	}Fpu;
	struct
	{
		ULONG64 Mm0;		// St0
		ULONG64 Reserved0;
		ULONG64 Mm1;		// St1
		ULONG64 Reserved1;
		ULONG64 Mm2;		// St2
		ULONG64 Reserved2;
		ULONG64 Mm3;		// St3
		ULONG64 Reserved3;
		ULONG64 Mm4;		// St4
		ULONG64 Reserved4;
		ULONG64 Mm5;		// St5
		ULONG64 Reserved5;
		ULONG64 Mm6;		// St6
		ULONG64 Reserved6;
		ULONG64 Mm7;		// St7
		ULONG64 Reserved7;
	}x87;
	NOIR_XMM_STATE Sse;
#if defined(_WIN64)
	ULONG64 Reserved[6];
#else
	ULONG64 Reserved[22];
#endif
	ULONG64 Available[6];
}NOIR_FX_STATE,*PNOIR_FX_STATE;

typedef struct _SEGMENT_REGISTER
{
	USHORT Selector;
	USHORT Attributes;
	ULONG32 Limit;
	ULONG64 Base;
}SEGMENT_REGISTER,*PSEGMENT_REGISTER;

typedef struct _NOIR_SR_STATE
{
	SEGMENT_REGISTER Es;
	SEGMENT_REGISTER Cs;
	SEGMENT_REGISTER Ss;
	SEGMENT_REGISTER Ds;
}NOIR_SR_STATE,*PNOIR_SR_STATE;

#define NoirMemoryTypeUncacheable		0
#define NoirMemoryTypeWriteCombining	1
#define NoirMemoryTypeWriteThrough		4
#define NoirMemoryTypeWriteProtect		5
#define NoirMemoryTypeWriteBack			6
#define NoirMemoryTypeUncachableMinus	7

typedef struct _NOIR_ADDRESS_MAPPING
{
	ULONG64 GPA;
	ULONG64 HVA;
	ULONG32 NumberOfPages;
	union
	{
		struct
		{
			ULONG32 Present:1;
			ULONG32 Write:1;
			ULONG32 Execute:1;
			ULONG32 User:1;
			ULONG32 Caching:3;
			ULONG32 PageSize:2;
			ULONG32 Reserved:23;
		};
		ULONG32 Value;
	}Attributes;
}NOIR_ADDRESS_MAPPING,*PNOIR_ADDRESS_MAPPING;

typedef enum _NOIR_CVM_REGISTER_TYPE
{
	NoirCvmGeneralPurposeRegister,
	NoirCvmFlagsRegister,
	NoirCvmInstructionPointer,
	NoirCvmControlRegister,
	NoirCvmCr2Register,
	NoirCvmDebugRegister,
	NoirCvmDr67Register,
	NoirCvmSegmentRegister,
	NoirCvmFsGsRegister,
	NoirCvmDescriptorTable,
	NoirCvmTrLdtrRegister,
	NoirCvmSysCallMsrRegister,
	NoirCvmSysEnterMsrRegister,
	NoirCvmCr8Register,
	NoirCvmFxState,
	NoirCvmXsaveArea,
	NoirCvmXcr0Register,
	NoirCvmMaxmimumRegisterType
}NOIR_CVM_REGISTER_TYPE,*PNOIR_CVM_REGISTER_TYPE;

typedef struct _NOIR_VIEW_EDIT_REGISTER_CONTEXT
{
	CVM_HANDLE VirtualMachine;
	ULONG32 VpIndex;
	NOIR_CVM_REGISTER_TYPE RegisterType;
	PVOID DummyBuffer;
}NOIR_VIEW_EDIT_REGISTER_CONTEXT,*PNOIR_VIEW_EDIT_REGISTER_CONTEXT;

typedef struct _NOIR_QUERY_ADBITMAP_CONTEXT
{
	CVM_HANDLE VirtualMachine;
	ULONG64 GpaStart;
	ULONG64 BitmapBuffer;
	ULONG32 BitmapLength;
	ULONG32 NumberOfPages;
}NOIR_QUERY_ADBITMAP_CONTEXT,*PNOIR_QUERY_ADBITMAP_CONTEXT;

typedef enum _NOIR_CVM_INTERCEPT_CODE
{
	CvInvalidState=0,
	CvShutdownCondition=1,
	CvMemoryAccess=2,
	CvInitSignal=3,
	CvHltInstruction=4,
	CvIoInstruction=5,
	CvCpuidInstruction=6,
	CvRdmsrInstruction=7,
	CvWrmsrInstruction=8,
	CvCrAccess=9,
	CvDrAccess=10,
	CvHypercall=11,
	CvException=12,
	CvRescission=13,
	CvInterruptWindow=14,
	CvSchedulerExit=0x80000000,
	CvSchedulerPause=0x80000001
}NOIR_CVM_INTERCEPT_CODE,*PNOIR_CVM_INTERCEPT_CODE;

typedef struct _NOIR_CVM_CR_ACCESS_CONTEXT
{
	struct
	{
		ULONG32 CrNumber:4;
		ULONG32 GprNumber:4;
		ULONG32 MovInstruction:1;
		ULONG32 Write:1;
		ULONG32 Reserved0:22;
	};
}NOIR_CVM_CR_ACCESS_CONTEXT,*PNOIR_CVM_CR_ACCESS_CONTEXT;

typedef struct _NOIR_CVM_DR_ACCESS_CONTEXT
{
	struct
	{
		ULONG32 DrNumber:4;
		ULONG32 GprNumber:4;
		ULONG32 Write:1;
		ULONG32 Reserved:23;
	};
}NOIR_CVM_DR_ACCESS_CONTEXT,*PNOIR_CVM_DR_ACCESS_CONTEXT;

typedef struct _NOIR_CVM_EXCEPTION_CONTEXT
{
	struct
	{
		ULONG32 Vector:5;
		ULONG32 EvValid:1;
		ULONG32 Reserved:26;
	};
	ULONG32 ErrorCode;
	ULONG64 PageFaultAddress;
	BYTE FetchedBytes;
	BYTE InstructionBytes[15];
}NOIR_CVM_EXCEPTION_CONTEXT,*PNOIR_CVM_EXCEPTION_CONTEXT;

typedef struct _NOIR_CVM_IO_CONTEXT
{
	struct
	{
		USHORT IoType:1;
		USHORT String:1;
		USHORT Repeat:1;
		USHORT OperandSize:3;
		USHORT AddressWidth:4;
		USHORT Reserved:6;
	}Access;
	USHORT Port;
	ULONG64 Rax;
	ULONG64 Rcx;
	ULONG64 Rsi;
	ULONG64 Rdi;
	SEGMENT_REGISTER Segment;
}NOIR_CVM_IO_CONTEXT,*PNOIR_CVM_IO_CONTEXT;

typedef struct _NOIR_CVM_MSR_CONTEXT
{
	ULONG32 Eax;
	ULONG32 Edx;
	ULONG32 Ecx;
}NOIR_CVM_MSR_CONTEXT,*PNOIR_CVM_MSR_CONTEXT;

typedef struct _NOIR_CVM_MEMORY_ACCESS_CONTEXT
{
	struct
	{
		BYTE Read:1;
		BYTE Write:1;
		BYTE Execute:1;
		BYTE User:1;
		BYTE FetchedBytes:4;
	}Access;
	BYTE InstructionBytes[15];
	ULONG64 Gpa;
}NOIR_CVM_MEMORY_ACCESS_CONTEXT,*PNOIR_CVM_MEMORY_ACCESS_CONTEXT;

typedef struct _NOIR_CVM_CPUID_CONTEXT
{
	struct
	{
		ULONG32 Eax;
		ULONG32 Ecx;
	}Leaf;
}NOIR_CVM_CPUID_CONTEXT,*PNOIR_CVM_CPUID_CONTEXT;

typedef struct _NOIR_CVM_EXIT_CONTEXT
{
	NOIR_CVM_INTERCEPT_CODE InterceptCode;
	union
	{
		NOIR_CVM_CR_ACCESS_CONTEXT CrAccess;
		NOIR_CVM_DR_ACCESS_CONTEXT DrAccess;
		NOIR_CVM_EXCEPTION_CONTEXT Exception;
		NOIR_CVM_IO_CONTEXT Io;
		NOIR_CVM_MSR_CONTEXT Msr;
		NOIR_CVM_MEMORY_ACCESS_CONTEXT MemoryAccess;
		NOIR_CVM_CPUID_CONTEXT Cpuid;
	};
	SEGMENT_REGISTER Cs;
	ULONG64 Rip;
	ULONG64 Rflags;
	ULONG64 NextRip;
	struct
	{
		ULONG32 Cpl:2;
		ULONG32 Pe:1;
		ULONG32 Lm:1;
		ULONG32 InterruptShadow:1;
		ULONG32 InstructionLength:4;
		ULONG32 Reserved:23;
	}VpState;
}NOIR_CVM_EXIT_CONTEXT,*PNOIR_CVM_EXIT_CONTEXT;

// Library Support Routine
BOOL NoirInitializeLibrary();
void NoirFinalizeLibrary();

// Support Functions
PVOID MemAlloc(IN SIZE_T Length);
BOOL MemFree(IN PVOID Memory);
PVOID PageAlloc(IN SIZE_T Length);
BOOL PageFree(IN PVOID Memory);
BOOL LockPage(IN PVOID Memory,IN ULONG Size);
BOOL UnlockPage(IN PVOID Memory,IN ULONG Size);

// Virtual Machine Management
NOIR_STATUS NoirCreateVirtualMachine(OUT PCVM_HANDLE VirtualMachine);
NOIR_STATUS NoirDeleteVirtualMachine(IN CVM_HANDLE VirtualMachine);

// Virtual Processor Management
NOIR_STATUS NoirCreateVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex);
NOIR_STATUS NoirDeleteVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex);

// VM Memory Management
NOIR_STATUS NoirSetAddressMapping(IN CVM_HANDLE VirtualMachine,IN PNOIR_ADDRESS_MAPPING MappingInformation);
NOIR_STATUS NoirClearGpaAccessingBits(IN CVM_HANDLE VirtualMachine,IN ULONG64 GpaStart,IN ULONG32 NumberOfPages);
NOIR_STATUS NoirQueryGpaAccessingBitmap(IN CVM_HANDLE VirtualMachine,IN ULONG64 GpaStart,IN ULONG32 NumberOfPages,OUT PVOID Bitmap,IN ULONG32 BitmapSize);

// Virtual Processor Registers
NOIR_STATUS NoirViewVirtualProcessorRegister(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN NOIR_CVM_REGISTER_TYPE RegisterType,OUT PVOID Buffer,IN ULONG32 BufferSize);
NOIR_STATUS NoirEditVirtualProcessorRegister(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN NOIR_CVM_REGISTER_TYPE RegisterType,IN PVOID Buffer,IN ULONG32 BufferSize);

// Virtual Processor Run-Control
NOIR_STATUS NoirRunVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,OUT PNOIR_CVM_EXIT_CONTEXT ExitContext);
NOIR_STATUS NoirRescindVirtualProcessor(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex);
NOIR_STATUS NoirSetEventInjection(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN BOOLEAN Valid,IN BYTE Vector,IN BYTE Type,IN BYTE Priority,IN BOOLEAN ErrorCodeValid,IN ULONG32 ErrorCode);
NOIR_STATUS NoirSetVirtualProcessorOptions(IN CVM_HANDLE VirtualMachine,IN ULONG32 VpIndex,IN NOIR_CVM_VIRTUAL_PROCESSOR_OPTION_TYPE Type,IN ULONG32 Data);