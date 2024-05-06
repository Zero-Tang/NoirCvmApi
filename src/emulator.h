#pragma once

#define PAGE_SIZE			0x1000
#define PAGE_OFFSET(x)		(x&0xfff)
#define PAGE_BASE(x)		(x&0xfffffffffffff000)
#define NEXT_PAGE_BASE(x)	(PAGE_BASE(x)+PAGE_SIZE)

#define PAGE_OVERFLOW(x,s)	((x+s)>NEXT_PAGE_BASE(x))

void __cdecl NoirDebugPrint(IN PCSTR Format,...);

NOIR_CVM_REGISTER_NAME StringIoRegisterNames[4]=
{
	NoirCvmRegisterRcx,
	NoirCvmRegisterRsi,
	NoirCvmRegisterRdi,
	NoirCvmRegisterRip
};

NOIR_CVM_REGISTER_NAME RegisterIoRegisterNames[2]=
{
	NoirCvmRegisterRax,
	NoirCvmRegisterRip
};