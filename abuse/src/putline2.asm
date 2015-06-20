
	.386
	.MODEL	small 

.code

; parm1 = scrn addr
; parm2 = remap addr
; parm3 = scrwidth

ALIGN 16
PUBLIC	remap_line_asm_
remap_line_asm_:

	push	ebp
	push	esi
	push	edi
	sub		esp, 32

	mov		esi, eax	; esi = screen addr
	mov		eax, edx	; eax = remap_table
	mov		edi, ebx	; edi = remap_seq
	mov		ebp, ecx	; ebp = remap_seq_len

	mov		ah, [edi]
	mov		ebx, eax
	jmp		remap_loop

ALIGN	16
remap_loop:

	mov		al, [esi]
	mov		bl, 1[esi]
	mov		al, [eax]
	mov		bl, [ebx]
	mov		[esi], al
	mov		1[esi], bl

	add		edi, 1
	mov		al, 2[esi]
	mov		bl, 3[esi]
	mov		al, [eax]
	mov		bl, [ebx]
	mov		2[esi], al
	mov		3[esi], bl

	mov		al, 4[esi]
	mov		bl, 5[esi]
	mov		al, [eax]
	add		esi, 8
	mov		bl, [ebx]
	mov		-4[esi], al
	mov		-3[esi], bl

	dec		ebp
	mov		al, -2[esi]
	mov		bl, -1[esi]
	mov		al, [eax]
	mov		bl, [ebx]
	mov		-2[esi], al
	mov		-1[esi], bl

	mov		ah, [edi]
	mov		ebx, eax
	jne		remap_loop

	add		esp, 32
	pop		edi
	pop		esi
	pop		ebp
	ret

; ENDP

END

