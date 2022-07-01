#!/bin/env python3

import os, sys, datetime

date = datetime.date.today()
_, inpath, outdir, app_name, inst_name = sys.argv

appvar_size = 60000

with open(inpath, 'rb') as infile:
	app_data = infile.read()

app_data += len(app_data).to_bytes(3, 'little')

installer_src_path = os.path.dirname(os.path.realpath(__file__)) + "/installer.bin"
installer_dest_path = f'{outdir}/{inst_name}.8xp'

os.system(f'convbin -i "{installer_src_path}" -o "{installer_dest_path}" -j bin -k 8xp -n {inst_name}')

segments = [app_data[x:x+appvar_size] for x in range(0, len(app_data), appvar_size)]
for (i, segment) in enumerate(segments):
	var_name = 'AppInst' + chr(ord('A') + i)
	filename_bin = outdir + '/' + var_name + '.bin'
	filename_8xv = outdir + '/' + var_name + '.8xv'
	with open(filename_bin, 'wb') as outfile:
		outfile.write(segment)
	os.system(f'convbin -i "{filename_bin}" -o "{filename_8xv}" -j bin -k 8xv -n {var_name} -r')
