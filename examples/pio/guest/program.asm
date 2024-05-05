bits 64

; Program Header
dq main			; Entry-Point
dq idt_base		; IDT Base
dq gdt_base		; GDT Base
dq tss_base		; TSS Base
dq pml4e_base	; Paging Base
dq 0

; Interrupt Handler
idt_base:
times 1024 db 0

; Program Segmentation
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

; Entry Point
main:
	; Test Case Calling Conventions:
	; rax, rdx, rcx, rsi, rdi: They follow standard x86 Port I/O.
	; rbx: This register specifies Port I/O Test Case ID.
	; rsp, rsp, r8-r15: Unused by hypervisor.
	cld
	; Test case 0 - Register Output
	xor ebx,ebx
	mov eax,'oleH'
	out 0,al
	out 0,ax
	out 0,eax
	; Test case 1 - Register Input
	inc ebx
	in eax,1
	out 0,eax
	; Test case 2 - String Output
	inc ebx
	mov ecx,dword [hello_text.len]
	lea rsi,hello_text
	rep outsb
	; Test case 3 - String Input
	inc ebx
	xor edx,edx
	mov ecx,20
	lea rdi,hello_text2
	rep insb
	mov ecx,20
	lea rsi,hello_text2
	rep outsb
	; End of Program
	hlt

hello_text:
	db "Hello Port I/O Test!",13,10
.len:
	dd .len-hello_text

hello_text2:
	times 20 db 0

times 4096-($-$$) db 0
; Page-Table
pml4e_base:
	dq pdpte_base+7			; Point to PDPTE
	times 511 dq 0
pdpte_base:
	dq pde_base+7			; Point to PDE
	times 511 dq 0
pde_base:
	dq 0x87,0x200087		; 2MiB Large Pages
	times 510 dq 0