bits 64
org 0x4000

header:
	; In the header section, we define a few essential stuff for initializing the vCPU.
	dq start
	dq syscall_handler
	dq gdt_base
	dq tss_base

gdt_base:
	; Put some segment information.
	dq 0,0	; Null segment
	; Kernel code segment is at 0x10
	dq 0x00209b0000000000
	; Data segment is at 0x18
	dq 0x00cff3000000ffff
	; User code segment is at 0x20
	dq 0x0020fb0000000000
	; Task register has selector 0x30
	dw 0x0067	; Segment Limit (Low)
	dw tss_base	; Segment Base (Low)
	db 0x00		; Segment Base (Low-Mid)
	dw 0x008b	; Segment Attributes
	db 0x00		; Segment Base (High-Mid)
	dd 0x00000000	; Segment Base (High)
	dd 0x00000000	; Reserved

tss_base:
	dd 0				; Reserved
	dq 0,0,0			; Rsp0-Rsp2
	dq 0				; Reserved
	dq 0,0,0,0,0,0,0	; IST0-IST7
	dq 0				; Reserved
	dw 0				; Reserved
	dw 0				; I/O Map Base Address

start:
	; Entry point. Note this is user mode.
	; All GPRs, except rsp, are zero.
	; The rflags.tf bit is assumed to be set from the beginning.
	nop			; We trigger the TF before syscall.
	; Then call into the kernel.
	syscall
	; When rax=1, calling into the hypervisor will shutdown the guest
	inc eax
	vmmcall
syscall_handler:
	; The handler of system call. From here on, it's kernel mode.
	nop
	vmmcall		; Call into the hypervisor.
	nop
	sysret		; Return to user mode.

