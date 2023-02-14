.code

extern handle_true_interrupt:proc
extern handle_false_interrupt:proc

push_msvc_abi macro

	sub rsp,38h
	mov qword ptr [rsp+00h],rax
	mov qword ptr [rsp+08h],rcx
	mov qword ptr [rsp+10h],rdx
	mov qword ptr [rsp+18h],r8
	mov qword ptr [rsp+20h],r9
	mov qword ptr [rsp+28h],r10
	mov qword ptr [rsp+30h],r11

endm

pop_msvc_abi macro

	mov rax,qword ptr [rsp+00h]
	mov rcx,qword ptr [rsp+08h]
	mov rdx,qword ptr [rsp+10h]
	mov r8,qword ptr [rsp+18h]
	mov r9,qword ptr [rsp+20h]
	mov r10,qword ptr [rsp+28h]
	mov r11,qword ptr [rsp+30h]
	add rsp,38h

endm

true_int_handler proc

	push_msvc_abi
	call handle_true_interrupt
	pop_msvc_abi
	iretq

true_int_handler endp

false_int_handler proc

	push_msvc_abi
	call handle_false_interrupt
	pop_msvc_abi
	iretq

false_int_handler endp

end