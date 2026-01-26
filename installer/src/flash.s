	.assume	adl=1
	.section	.text

	.global _flash_erase
_flash_erase:
	ld	iy,$d00080
	push	ix
	ld	ix,0
	add	ix,sp
	call	_port_unlock
	ld	a,(ix+6)
	call	.L.wrap
	call	_port_lock
	pop	ix
	ret
.L.wrap:
	ld	bc,$f8
	push	bc
	jp	$02dc

	.global _flash_write
_flash_write:
	ld	iy,$d00080
	push	ix
	ld	ix,0
	add	ix,sp
	ld	de,(ix+6)
	ld	hl,(ix+9)
	ld	bc,(ix+12)
	call	_port_unlock
	call	$02e0
	call	_port_lock
	pop	ix
	ret
