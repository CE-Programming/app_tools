	.assume	adl=1
	.section	.text

	.equ	ti.StrCmpre, 0x021DB0
	.equ	ti._frameset0, 0x000130
	.equ	ti.textShadow, 0xD006C0
	.equ	ti.CheckIfEmulated, 0x000578
	.equ 	ti.KeypadScanFull, 0x0003D4
	.equ 	ti._indcall, 0x00015C

	.global	_port_setup
_port_setup:
	push ix
	call port_setup
	pop ix
	ret

port_setup:
	di
	ld	hl,port_reloc
	ld	bc,port_reloc.size
	ld	de,___port_reloc
	ldir
	ld hl, ($000008+5)
	ld a, (hl)
	cp a, $CD
	ret nz
	inc hl
	ld de, (hl)
	ld hl, (ti.CheckIfEmulated+1)
	sbc hl, de
	ret nz
	ld hl, (ti.KeypadScanFull+1)
	ld bc, 10
	add hl, bc
	push hl
	ld b, port_pattern.size
	ld de, port_pattern
	call ti.StrCmpre
	pop hl
	ret nz
	ld (___port_unlock.target), hl
	xor a, a
	ret

port_pattern:
	.db $ED,$79,$78,$FE,$A0,$28,$01,$CF
	.equ port_pattern.size, $-port_pattern 

	.global _port_unlock
_port_unlock:
	push ix
	push de
	push bc
	push hl
	call ___port_unlock
	jr _port_lock.pop

	.global _port_lock
_port_lock:
	push ix
	push de
	push bc
	push hl
	call ___port_lock

_port_lock.pop:
	pop hl
	pop bc
	pop de
	pop ix
	ret

port_reloc:
	.equ	___port_reloc, ti.textShadow
	.equ	___port_unlock, ___port_reloc
	call ti._frameset0
	ld iy, ___port_unlock.unlockfinish
	ld sp, ti._indcall + 7
	ld bc, $22
	xor a, a

	.equ ___port_unlock.target, $+1
	jp 0

	.equ	___port_unlock.unlockfinish, ___port_reloc + ($-port_reloc)
	ld sp, ix
	pop ix
	ld a, $8C
	out0 ($24), a
	in0 a, ($06)
	or a, 4
	out0 ($06), a
	ret

	.equ	___port_lock, ___port_reloc + ($-port_reloc)
	xor a, a
	out0 ($28), a
	in0 a, ($06)
	res 2, a
	out0 ($06), a
	ld a, $88
	out0 ($24), a
	ld a, $D1
	out0 ($22), a
	ret
	.equ	port_reloc.size, $-port_reloc
