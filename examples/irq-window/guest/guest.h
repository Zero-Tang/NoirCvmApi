#pragma once

typedef unsigned __int8		u8;
typedef unsigned __int16	u16;
typedef unsigned __int32	u32;
typedef unsigned __int64	u64;

typedef signed __int8		i8;
typedef signed __int16		i16;
typedef signed __int32		i32;
typedef signed __int64		i64;

typedef u8*		u8p;
typedef u16*	u16p;
typedef u32*	u32p;
typedef u64*	u64p;

typedef i8*		i8p;
typedef i16*	i16p;
typedef i32*	i32p;
typedef i64*	i64p;

#define kernel_code_sel		0x10
#define kernel_data_sel		0x18
#define kernel_task_sel		0x20

#define descriptor_64bit_interrupt_gate	0xE

#define int_vec_true	0x30
#define int_vec_false	0x40

#define stdin_port	0
#define stdout_port	1
#define stderr_port	2

typedef struct _dt_reg
{
	u16 pad[3];
	u16 limit;
	u64 base;
}dt_reg,*dt_reg_p;

typedef struct _idt_entry
{
	u16 offset_lo;
	u16 selector;
	union
	{
		struct
		{
			u16 ist:3;
			u16 reserved:5;
			u16 type:5;
			u16 dpl:2;
			u16 present:1;
		};
		u16 value;
	}flags;
	u16 offset_mid;
	u32 offset_hi;
	u32 reserved;
}idt_entry,*idt_entry_p;

void true_int_handler(void);
void false_int_handler(void);