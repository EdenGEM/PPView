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


def ErrorSummary():
	global kEnv
	
	conn=MySQLdb.connect(host='10.10.151.68',user='reader',passwd='miaoji1109',db='ppview',port=3306,charset="utf8")
	cur=conn.cursor()
#	sqlstr = 'replace into ppview.ppview_chat_test_summary (type, date , error_id, error_num) select type, date, error_id, count(*) from ppview.ppview_chat_test_log group by error_id,type'
	sqlstr = "replace into ppview.ppview_%s_summary (type, date , error_id, error_num) select type, date, error_id, count(*) from ppview.ppview_%s_log where date = '%s' group by error_id,type;" % (kEnv,kEnv,kDate)
#	return 
	try:
		cur.execute(sqlstr)
		conn.commit()
		cur.close()
		conn.close()
	except MySQLdb.Error,e:
		print >> sys.stderr, "Mysql Error %d: %s" % (e.args[0], e.args[1])

kDate = '20150101'

def main():
	global kDate,kEnv
	
	parser = optparse.OptionParser()
	parser.add_option('-d', '--date', help = 'run date like 20150101')
	parser.add_option('-e', '--env', help = 'run date like test')
	opt, args = parser.parse_args()
	try:	
		run_day = datetime.datetime.fromtimestamp(time.mktime(time.strptime(opt.date, '%Y%m%d')))
		date_str = run_day.strftime('%Y%m%d')
		kDate = date_str;
	except:
		print "Error Format date"
		return
	if opt.env not in ("chat_test","chat_online", "md_test", "md_online"):
		print "Error Format environment"
		return
	else:
		kEnv=opt.env
	ErrorSummary();

if __name__ == '__main__':
	main()

