#!/usr/bin/env python
#coding=utf-8

import time
import datetime
import os
import os.path
import json
import MySQLdb
import re
import sys
import optparse


kDate = '20150101'

def main():
	global kDate
	
	parser = optparse.OptionParser()
	parser.add_option('-d', '--date', help = 'run date like 20150101')
	opt, args = parser.parse_args()
	try:	
		run_day = datetime.datetime.fromtimestamp(time.mktime(time.strptime(opt.date, '%Y%m%d')))
		date_str = run_day.strftime('%Y%m%d')
		kDate = date_str;
	except:
		print "%s is not a date" % (opt.date)
		sys.exit(1)
	sys.exit(0)

if __name__ == '__main__':
	main()

