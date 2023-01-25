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
	dq 0x87,0x200087	; 2MiB Large Pages
	times 510 dq 0

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

; The Guest Program starts at 0x5000
program:
	; Get version.
	mov eax,dword [0x200000]
	cld	; Initialize printer direction.
	; Print Version Text
	mov ecx,vertext_length
	mov rsi,version_text
	call write_console_text
	; Print Version Text
	call write_version_text
	; Line feed
	mov byte [0x200004],0x0d
	mov byte [0x200004],0x0a
	; Print Hello Text
	mov ecx,text_length
	mov rsi,hello_text
	call write_console_text
	; Shutdown the guest.
	mov dword [0x200008],0xaa55

; Calling Convention:
; rsi: pointer to the start of string
; rcx: length of the string.
; rflags.df: direction of printing.
; Return:
; rsi: pointer to the end of string
; rcx: zero
; al: last character
write_console_text:
	lodsb
	mov byte [0x200004],al
	loop write_console_text
	ret

; Calling convention
; r8b: number of character positions (use character)
; r9b: direction of movement (A for upward, B for downward, C for forward, D for backward)
move_cursor:
	mov byte [0x200004],0x1b
	mov byte [0x200004],'['
	mov byte [0x200004],r8b
	mov byte [0x200004],r9b
	ret

; Calling Convention:
; eax: the version number; destroyed on return
write_version_text:
	push rcx
	push rdx
	mov r8b,'7'
	mov r9b,'C'
	call move_cursor
	mov ecx,8
.loop_start:
	mov edx,eax
	and edx,0xF
	shr eax,4
	cmp dl,10
	jb .num
	add dl,'A'
	jmp .print
.num:
	add dl,'0'
.print:
	mov r8b,'2'
	mov r9b,'D'
	mov byte [0x200004],dl
	call move_cursor
	loop .loop_start
	mov r8b,'7'
	mov r9b,'C'
	call move_cursor
	pop rdx
	pop rcx
	ret

version_text:
	db "Version: 0x"

vertext_length equ $-version_text

hello_text:
	; Set console to violet foreground
	db 0x1b,"[38;2;123;46;233m"
	; Violet Text
	db "Hello, this is Violet!",0x0d,0x0a
	; Set console to cyan foreground
	db 0x1b,"[96m"
	; Cyan Text
	db "Hello, this is Cyan!",0x0d,0x0a,0x0

text_length equ $-hello_text