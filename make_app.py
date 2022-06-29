#!env python3

import sys, datetime

app_name = sys.argv[3]
date = datetime.date.today()

with open(sys.argv[1], 'rb') as infile, open(sys.argv[2], 'wb') as outfile:
	app_data = infile.read()
	outfile.write(b'**TIFL**\x05\x00\x00\x00')
	outfile.write(bytes.fromhex(f"{date.day:02}{date.month:02}{date.year}"))
	outfile.write(bytes([len(app_name)]))
	outfile.write(app_name.encode('utf-8') + b'\x00' * (8 - len(app_name)))
	outfile.write(b'\x00' * 23)
	outfile.write(b'\x73\x24')
	outfile.write(b'\x00' * 23)
	outfile.write(b'\x13') # todo: what does this mean?
	outfile.write(len(app_data).to_bytes(4, 'little'))
	outfile.write(app_data)
