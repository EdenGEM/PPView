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
from DBHandle import DBHandle

reload(sys);
exec("sys.setdefaultencoding('utf-8')");

report = dict()
error_dict = dict()

def NoReport():
	print "<html><body><h1>No Data</h1></body></html>"

def Report():
	
	conn=DBHandle("127.0.0.1","","","test")
	sqlstr='select qid,8003stat from Traf8003 where date="%s";'%kDate
#	sqlstr='select sum(total),sum(taxi_sphere),sum(taxi_google),sum(taxi_cache),sum(taxi_gc),sum(walk_sphere),sum(walk_google),sum(walk_cache),sum(walk_gc),sum(public_sphere),sum(public_google),sum(public_cache),sum(public_gc),sum(sphere_twp),sum(google_twp),sum(cache_twp),sum(twp_gc) from test.interface8003 where date=%s;'%kDate
	rows=conn.do(sqlstr)
	if len(rows) == 0:
		NoReport()
		return 
	total=0
	taxi_sphere=0
	taxi_google=0
	taxi_cache=0
	taxi_gc=0
	walk_sphere=0
	walk_google=0
	walk_cache=0
	walk_gc=0
	public_sphere=0
	public_google=0
	public_cache=0
	public_gc=0
	sphere_twp=0
	google_twp=0
	cache_twp=0
	twp_gc=0
	totalTraf=0
	realTraf=0
	notRealTraf=0

	for row in rows:
		stat=row["8003stat"]
		stat=json.loads(stat)
		total+=stat["total"]
		taxi_sphere+=stat["taxi_sphere"]
		taxi_google+=stat["taxi_google"]
		taxi_cache+=stat["taxi_cache"]
		taxi_gc+=stat["taxi_gc"]
		walk_sphere+=stat["walk_sphere"]
		walk_google+=stat["walk_google"]
		walk_cache+=stat["walk_cache"]
		walk_gc+=stat["walk_gc"]
		public_sphere+=stat["public_sphere"]
		public_google+=stat["public_google"]
		public_cache+=stat["public_cache"]
		public_gc+=stat["public_gc"]
		sphere_twp+=stat["sphere_twp"]
		google_twp+=stat["google_twp"]
		cache_twp+=stat["cache_twp"]
		twp_gc+=stat["twp_gc"]
		totalTraf+=stat["totalTraf"]
		realTraf+=stat["realTraf"]
		notRealTraf+=stat["notRealTraf"]
	print '<html><body>'
	print '<h1 align="center">8003 online归总日志分析</h1>'
	print '<table border="1" align="center">'

	print '<tr><th>total pair(',str(total),')</th><th>sphere总数(占比)</th><th>google总数(占比)</th><th>cache总数(占比)</th><th>google_or_cache总数(占比)</th></tr>'
	print '<tr>'
	print '<th>taxi</th><td>',str(taxi_sphere),'(',("%.2f"%(float(taxi_sphere)/total*100)),'%)</td><td>',str(taxi_google),'(',('%.2f'%(float(taxi_google)/total*100)),'%)</td><td>',str(taxi_cache),'(',("%.2f"%(float(taxi_cache)/total*100)),'%)</td><td>',str(taxi_gc),'(',('%.2f'%(float(taxi_gc)/total*100)),'%)</td></tr>'
	print '<tr>'
	print '<th>walk</th><td>',str(walk_sphere),'(',('%.2f'%(float(walk_sphere)/total*100)),'%)</td><td>',str(walk_google),'(',('%.2f'%(float(walk_google)/total*100)),'%)</td><td>',str(walk_cache),'(',('%.2f'%(float(walk_cache)/total*100)),'%)</td><td>',str(walk_gc),'(',('%.2f'%(float(walk_gc)/total*100)),'%)</td></tr>'
	print '<tr>'
	print '<th>public</th><td>',str(public_sphere),'(',('%.2f'%(float(public_sphere)/total*100)),'%)</td><td>',str(public_google),'(',('%.2f'%(float(public_google)/total*100)),'%)</td><td>',str(public_cache),'(',('%.2f'%(float(public_cache)/total*100)),'%)</td><td>',str(public_gc),'(',('%.2f'%(float(public_gc)/total*100)),'%)</td></tr>'
	print '<tr>'
	print '<th>t/w/p</th><td>',str(sphere_twp),'(',('%.2f'%(float(sphere_twp)/total*100)),'%)</td><td>',str(google_twp),'(',('%.2f'%(float(google_twp)/total*100)),'%)</td><td>',str(cache_twp),'(',('%.2f'%(float(cache_twp)/total*100)),'%)</td><td>',str(twp_gc),'(',('%.2f'%(float(twp_gc)/total*100)),'%)</td></tr>'
	print '</table>'
	print '<br>'
	print '<br>'
	print '<table border="1" align="center">'
	print '<tr><th>total Replace Traffic</th><th>Success Replace(占比)</th><th>Fail Replace(占比)</th></tr>'
	print '<tr><td>',totalTraf,'</td><td>',realTraf,'(',('%.2f'%(float(realTraf)/totalTraf*100)),'%)</td><td>',notRealTraf,'(',('%.2f'%(float(notRealTraf)/totalTraf*100)),')</td></tr>'
	print '</table>'
	print '<br>'
	print '<p align="center">'
	print '<br> <b>sphere:</b> 实时构造的假交通,距离为球面直线距离'
	print '<br> <b>google:</b> 爬虫实时抓取的google地图交通数据'
	print '<br> <b>cache:</b> 静态数据，google抓取的缓存数据或是离线规划(现在只有日本)的交通数据 '
	print '<br> <b>t/w/p:</b>如sphere列，交通点对中出现了sphere的数量'
	print '<br> <b>total pair:</b> 总的交通点对数'
	print '</p>'
	print "<body><html>"
	
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
#		print "kDate=%s"%kDate
	except:
		print "Error Format date"
		return
	Report();

if __name__ == '__main__':
	main()

