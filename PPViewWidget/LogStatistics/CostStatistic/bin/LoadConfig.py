#!/usr/bin/env python
# *-* coding:utf8 *-*

import PrintInfo
import sys

k_dic_config = dict()

def LoadConfig(file_name):
	global k_dic_config
	f = None
	try:
		f = open(file_name, 'r');
	except Exception,e:
		PrintInfo.PrintWarning("Open %s fail" % (file_name));	
		return 1
	while True:
		line = f.readline()
		if not line:
			break
		if line == "":
			continue
		if line.startswith('#'):
			continue
		key_value = line.split('=')
		if len(key_value) != 2:
			continue
		k_dic_config[key_value[0].strip()] = key_value[1].strip()
	if f:
		f.close()
		f = None
	return 0

def main():
	if len(sys.argv) < 2:
		PrintInfo.PrintWarning("argv nums err, exit")
		return 
	ret = LoadConfig(sys.argv[1]);
	if ret > 0:
		exit(1)
	for key in k_dic_config:
		PrintInfo.PrintLog("key: %s -----> value: %s" % (key, k_dic_config[key]))

if __name__ == "__main__":
	main()

