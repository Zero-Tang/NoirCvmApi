#include "guest.h"

void handle_true_interrupt()
{
	char msg1[]="True interrupt...\n";
	char msg2[]="Over...\n";
	__outbytestring(stdout_port,msg1,sizeof(msg1));
	_enable();		// At this point, we should receive an interrupt.
	__nop();
	_disable();
	__outbytestring(stdout_port,msg2,sizeof(msg2));
}

void handle_false_interrupt()
{
	char msg[]="False interrupt...\n";
	__outbytestring(stdout_port,msg,sizeof(msg));
}

void static set_idt_entry(idt_entry_p idt,u8 vector,void* handler_routine)
{
	u64 hr_val=(u64)handler_routine;
	idt[vector].offset_lo=(u16)hr_val;
	idt[vector].selector=kernel_code_sel;
	idt[vector].flags.ist=0;
	idt[vector].flags.reserved=0;
	idt[vector].flags.type=descriptor_64bit_interrupt_gate;
	idt[vector].flags.dpl=0;
	idt[vector].flags.present=1;
	idt[vector].offset_mid=(u16)(hr_val>>16);
	idt[vector].offset_hi=(u32)(hr_val>>32);
	idt[vector].reserved=0;
}

void static init_idt()
{
	dt_reg idtr;
	idt_entry_p idt_base;
	__sidt(&idtr.limit);
	idt_base=(idt_entry_p)idtr.base;
	set_idt_entry(idt_base,int_vec_true,true_int_handler);
	set_idt_entry(idt_base,int_vec_false,false_int_handler);
}

void guest_entry()
{
	char a[]="Hello!\n";
	init_idt();
	__outbytestring(stdout_port,a,sizeof(a));
	_enable();
	__nop();	// The sti instruction causes an interrupt-shadow.
	_disable();
	__halt();
}