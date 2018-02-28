#!/usr/bin/env python 
#*-* coding : utf8 *-*
import urllib
import re

class Config:
	def __init__(self, filename):
		self.filename = filename;
	
	def LoadConfig():
		f = None;
		try:
			f = open(self.filename)
			while True:
				line = f.readline()
				if not line:
					break;
				if line.startswith('#'):
					continue
				if line == '':
					continue;
	def Analyze(line):
		key_value = line.split('=')
		if len(key_value) != 2:
			continue;
		config_dict[key_value[0].strip()] = key_value[1].strip()
		if key_value[0] == 'environment':
			self.environment = key_value[1].strip();
		else if key_value[0] == 'log_dir':e
			selt	.environment = key_value[1].strip();
		except Exception,e:
			print >> stderr, e;
		finally:
			if f:
				f.close();

	





