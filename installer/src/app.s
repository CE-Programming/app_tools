	.assume	adl=1
	.section	.text

	.global _find_last_app
_find_last_app:
	ld	hl,$3b0000		; applications start here
.L.find:
	push	hl
	call	$22044			; locates start of next application header
	pop	de
	jr	nz,.L.find
	ex	de,hl
	ret

	.global _fix_relocations
_fix_relocations:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl,(ix+6)

	push	hl
	ld	de,$112
	add	hl,de
	ld	hl,(hl)			; hl = code offset
	ld	de,$100
	add	hl,de
	pop	de
	add	hl,de
	ex	de,hl			; de = code section start

	ld	bc,$12a
	add	hl,bc			; hl = relocation table start
.L.relocate:
	or	a,a
	sbc	hl,de
	add	hl,de
	jr	z,.L.endrelocate

	ld	ix,(hl)
	add	ix,de			; location to overwrite
	inc	hl
	inc	hl
	inc	hl			; hl points to value | base
	push	hl
	push	de
	ld	hl,(hl)

	ld	bc,$800000		; check if data or code relative
	or	a,a
	sbc	hl,bc
	add	hl,bc
	jr	c,.L.coderelative
	ld	de,$d1787c - $800000
.L.coderelative:
	add	hl,de
	lea	de,ix + 0
	ld	($d02655),hl		; use tempFreeArc since it is spare ram
	ld	hl,$d02655
	ld	bc,3
	ld	iy,$d00080
	call	_port_unlock
	call	$02e0
	call	_port_lock
	pop	de
	pop	hl
	inc	hl
	inc	hl
	inc	hl
	jr	.L.relocate
.L.endrelocate:
	xor	a,a
	pop	ix
	ret

	.global _confirm
	.type _confirm, @function

	.equ	ti.flags, 0xD00080
	.equ	ti.curRow, 0xD00595
	.equ	ti.curCol, 0xD00596
	.equ	ti.textInverse, 3
	.equ	ti.textFlags, 5
	.equ	ti.ClearRect, 0x021218
	.equ	ti.PutS, 0x0207C0
	.equ	ti.GetCSC, 0x02014C
	.equ	ti.skRight, 0x03
	.equ	ti.skLeft, 0x02
	.equ	ti.skClear, 0x0F
	.equ	ti.skEnter, 0x09

; Shows Yes/No on the current row, returning 1 for Yes and 0 for No.
_confirm:
	ld	iy, ti.flags
	ld	a, (ti.curRow)
	push	af
	res	ti.textInverse, (iy + ti.textFlags)
.L.update_display:
	pop	af
	push	af
	ld	(ti.curRow), a
	ld	b, a
	add	a, a
	add	a, a
	add	a, b
	add	a, a
	add	a, a
	add	a, 37
	ld	b, a
	add	a, 17
	ld	c, a
	ld	hl, 72
	ld	de, 253
	call	ti.ClearRect
	ld	hl, 6
	ld.sis	(ti.curCol & $FFFF), hl
	ld	hl, .L.option_yes
	call	ti.PutS
	ld	hl, 16
	ld.sis	(ti.curCol & $FFFF), hl
	ld	a, (iy + ti.textFlags)
	xor	a, 8
	ld	(iy + ti.textFlags), a
	ld	hl, .L.option_no
	call	ti.PutS
.L.get_key:
	call	ti.GetCSC
	cp	a, ti.skRight
	jr	z, .L.update_display
	cp	a, ti.skLeft
	jr	z, .L.update_display
	cp	a, ti.skClear
	jr	z, .L.exit
	cp	a, ti.skEnter
	jr	nz, .L.get_key
	pop	bc
	bit	ti.textInverse, (iy + ti.textFlags)
	res	ti.textInverse, (iy + ti.textFlags)
	ld	a, 1
	ret	z
	xor	a, a
	ret
.L.exit:
	pop	bc
	res	ti.textInverse, (iy + ti.textFlags)
	xor	a, a
	ret
.L.option_yes:
	db	" Yes ", 0
.L.option_no:
	db	" No ", 0

	.global _delete_app
	.type _delete_app, @function

	.equ	ti.DeleteApp, 0x02126C

; Marks the app as deleted; its space is reclaimed when the OS next defragments.
_delete_app:
	pop	de
	pop	hl
	push	hl
	push	de
	ld	iy, ti.flags
	jp	ti.DeleteApp

	.global _garbage_collect
	.type _garbage_collect, @function

	.equ	ti.CreateProtProg, 0x021334
	.equ	ti.ParseInp, 0x020F00
	.equ	ti.PushErrorHandler, 0x020798
	.equ	ti.PopErrorHandler, 0x02079C
	.equ	ti.newDispF, 0x08
	.equ	ti.allowProgTokens, 1
	.equ	ti.cmdFlags, 0x0C
	.equ	ti.cmdExec, 6

; Runs the GarbageCollect command, which asks the user before collecting.
_garbage_collect:
	push	ix
	ld	iy, ti.flags
	call	.L.delete_gc_prgm
	ld	a, (iy + ti.newDispF)
	ld	c, (iy + ti.cmdFlags)
	push	af
	push	bc
	ld	hl, .L.gc_error
	call	ti.PushErrorHandler
	ld	hl, .L.gc_prgm
	call	ti.Mov9ToOP1
	ld	hl, 2
	call	ti.CreateProtProg
	inc	de
	inc	de
	ld	a, $BB
	ld	(de), a
	inc	de
	ld	a, $CE
	ld	(de), a
	set	ti.allowProgTokens, (iy + ti.newDispF)
	set	ti.cmdExec, (iy + ti.cmdFlags)
	ld	hl, .L.gc_prgm
	call	ti.Mov9ToOP1
	call	ti.ParseInp
	call	ti.PopErrorHandler
.L.gc_error:
	ld	iy, ti.flags
	pop	bc
	pop	af
	ld	(iy + ti.newDispF), a
	ld	(iy + ti.cmdFlags), c
	call	.L.delete_gc_prgm
	pop	ix
	ret
.L.delete_gc_prgm:
	ld	hl, .L.gc_prgm
	call	ti.Mov9ToOP1
	call	ti.ChkFindSym
	ret	c
	jp	ti.DelVarArc
.L.gc_prgm:
	db	6, "#GC", 0, 0, 0, 0, 0

	.global _delete_var
	.type _delete_var, @function

	.equ	ti.Mov9ToOP1, 0x020320
	.equ	ti.ChkFindSym, 0x02050C
	.equ	ti.DelVarArc, 0x021434
	.equ	ti.OP1, 0xD005F8

_delete_var:
	ld	iy, ti.flags
	pop	de
	pop	hl
	pop	bc
	push	bc
	push	hl
	push	de
	ld	a, c
	dec	hl
	push	af
	call	ti.Mov9ToOP1
	pop	af
	ld	(ti.OP1), a
	call	ti.ChkFindSym
	ret	c
	jp	ti.DelVarArc
