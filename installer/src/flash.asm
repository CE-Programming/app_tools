section .text

_flash_erase:
	ld	iy,$d00080
	push	ix
	ld	ix,0
	add	ix,sp
	call	_port_unlock
	ld	a,(ix+6)
	call	.wrap
	call	_port_lock
	pop	ix
	ret
.wrap:
	ld	bc,$f8
	push	bc
	jp	$02dc

public _flash_erase

section .text

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

public _flash_write

extern _port_unlock
extern _port_lock
