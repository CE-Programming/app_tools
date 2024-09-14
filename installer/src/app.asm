; this should probably go in a linker script but whatever
section .text
_os_FindAppStart = $21100
public _os_FindAppStart

section .text

_find_last_app:
	ld	hl,$3b0000		; applications start here
.find:
	push	hl
	call	$22044			; locates start of next application header
	pop	de
	jr	nz,.find
	ex	de,hl
	ret

public _find_last_app

section .text

table_offset = $12a
data_loc = $d1787c

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

	ld	bc,table_offset
	add	hl,bc			; hl = relocation table start
.relocate:
	or	a,a
	sbc	hl,de
	add	hl,de
	jr	z,.endrelocate

	ld	ix,(hl)
	add	ix,de			; location to overwrite
	inc	hl
	inc	hl
	inc	hl			; hl points to value | base
	push	hl
	push	de
	ld	hl,(hl)

	ld	bc,$800000		; check if data- or code- relative
	or	a,a
	sbc	hl,bc
	add	hl,bc
	jr	c,.coderelative
	ld	de,data_loc - $800000
.coderelative:
	add	hl,de
	lea	de,ix
	ld	($d0062f),hl
	ld	hl,$d0062f
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
	jr	.relocate
.endrelocate:
	xor	a,a
.exit:
	pop	ix
	ret

public _fix_relocations

extern _port_unlock
extern _port_lock