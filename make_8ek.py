#!/bin/env python3

import os, sys, datetime

date = datetime.date.today()
_, inpath, outpath, app_name = sys.argv

def gen_8ek(app_data, app_name):
	return b''.join([
		b'**TIFL**\x05\x00\x00\x00',
		bytes.fromhex(f"{date.day:02}{date.month:02}{date.year}"),
		bytes([len(app_name)]),
		app_name.encode('utf-8') + b'\x00' * (8 - len(app_name)),
		b'\x00' * 23,
		b'\x73\x24',
		b'\x00' * 23,
		b'\x13', # todo: what does this mean?
		len(app_data).to_bytes(4, 'little'),
		app_data
	])

with open(inpath, 'rb') as infile:
	app_data = infile.read()

with open(outpath, 'wb') as outfile:
	outfile.write(gen_8ek(app_data, app_name))

