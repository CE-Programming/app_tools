section .text

_app_name:
	db	app_name,0

virtual
	file	__app_bin
	dl	$ - $$

	load	app_name:8 from $$ + 30
	total_length = $ - $$
end virtual

MAX_APPVAR_SIZE = 65232

_segments:
pos = 0
var_suffix = 0
while pos < total_length
	size = MAX_APPVAR_SIZE
	if pos + size > total_length
		size = total_length - pos
	end if
	db	$15 ; appvar type
	db	__appvar_prefix
	if var_suffix < 10
		db	'0'
	end if
	repeat 1, d:var_suffix
		db	`d
	end repeat
	emit	7 - lengthof __appvar_prefix: 0
	dw	0 ; file offset
	dl	pos ; app offset
	dl	size
	rl	1 ; src pos

	var_suffix = var_suffix + 1
	pos = pos + size
end while
db	0 ; terminator

public _app_name
public _segments
extern __app_bin
extern __appvar_prefix
