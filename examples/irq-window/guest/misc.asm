; Paging Structure of Guest Program
bits 64
org 0x0

pml4e_base:
	dq 0x1007		; Point to PDPTE
	times 511 dq 0
pdpte_base:
	dq 0x2007		; Point to PDE
	times 511 dq 0
pde_base:
	dq 0x87			; 2MiB Large Pages
	times 511 dq 0

gdt_base:
	; Put some segment information.
	dq 0,0	; Null segment
	; Kernel code segment is at 0x10
	dq 0x00209b0000000000
	; Kernel data segment is at 0x18
	dq 0x00cf93000000ffff
	; Task register has selector 0x20
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
	times 0x5000-($-$$) db 0