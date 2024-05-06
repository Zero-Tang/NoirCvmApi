bits 64

%define test_port	0x00
%define dbg_port	0xE9

; Program Header
dq main			; Entry-Point
dq idt_base		; IDT Base
dq gdt_base		; GDT Base
dq tss_base		; TSS Base
dq pml4e_base	; Paging Base
dq 0

; Interrupt Handler
idt_base:
times 14*2 dq 0
; #PF Handler
dw pf_handler	; Low 16 bits
dw 0x10			; Selector
db 0x00			; No IST
db 0x8E			; Present, DPL=0, Interrupt Gate
dw 0x0000		; Mid 16 bits
dd 0			; High 32 bits
times 240*2 dq 0

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
	mov eax,'Helo'
	out 0,al
	out 0,ax
	out 0,eax
	; Test case 1 - Register Input
	inc ebx
	in al,0
	inc eax
	out 0,eax
	in ax,0
	add ax,0x101
	out 0,eax
	in eax,0
	add eax,0x1010101
	out 0,eax
	; Test case 2 - String Output
	inc ebx
	mov ecx,dword [hello_text.len]
	lea rsi,hello_text
	rep outsb
	; Test case 3 - String Input
	inc ebx
	xor edx,edx
	mov ecx,26
	lea rdi,hello_text2
	rep insb
	mov ecx,26
	lea rsi,hello_text2
	rep outsb
	; Test case 4 - Reversed String I/O
	inc ebx
	std
	mov ecx,26
	dec rsi
	rep outsb
	; Test case 5 - Page Fault
	inc ebx
	xor esi,esi
	not rsi
	outsb
	; If we reach here, #PF is not correctly handled.
	lea rsi,no_pf_happened
	mov ecx,dword [no_pf_happened.len]
	call debug_print
	; End of Program
	hlt

pf_handler:
	; Check if #PF happened at test case 5.
	cmp ebx,5
	jz .correct
	lea rsi,no_pf_happened
	mov ecx,dword [no_pf_happened.len]
	call debug_print
	jmp .end
.correct:
	lea rsi,pf_happened
	mov ecx,dword [pf_happened.len]
	call debug_print
.end:
	hlt

; rsi: pointer to string
; rcx: length of string
; All registers will be preserved
debug_print:
	; Save registers.
	pushf
	push rcx
	push rax
	cld
.loop_out:
	lodsb
	out dbg_port,al
	loop .loop_out
	; Restore registers.
	pop rax
	pop rcx
	; rsi can be restored via subtraction.
	sub rsi,rcx
	popf
	ret

hello_text:
	db "Hello Port I/O Test!",0
.len:
	dd .len-hello_text

no_pf_happened:
	db "Incorrect #PF!",13,10
.len:
	dd .len-no_pf_happened

pf_happened:
	db "Correct #PF!",13,10
.len:
	dd .len-pf_happened

hello_text2:
	times 26 db 0

times 0x2000-($-$$) db 0
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