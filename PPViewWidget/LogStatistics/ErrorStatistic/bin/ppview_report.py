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

reload(sys);
exec("sys.setdefaultencoding('utf-8')");

report = dict()
error_dict = dict()

def NoReport():
	print "<html><body><h1>No Data</h1></body></html>"

def Report():
	global kEnv
	
	conn=MySQLdb.connect(host='10.10.151.68',user='reader',passwd='miaoji1109',db='ppview',port=3306, charset="utf8")
	cur=conn.cursor()
#	sqlstr = 'replace into ppview.ppview_chat_test_summary (type, date , error_id, error_num) select type, date, error_id, count(*) from ppview.ppview_chat_test_log group by error_id,type'
#	sqlstr = "replace into ppview.ppview_chat_test_summary (type, date , error_id, error_num) select type, date, error_id, count(*) from ppview.ppview_chat_test_log where date = '%s' group by error_id,type;" % (kDate)
#	print sqlstr
#	return 
	try:
		sqlstr = "select SUM(error_num) from ppview.ppview_chat_%s_summary  where date = '%s';" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if not rows[0][0]:
			NoReport()
			return
		sum_req = int(rows[0][0]);

		sqlstr = "select SUM(error_num) from ppview.ppview_chat_%s_summary  where date = '%s' and error_id = 0;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		sum_req_normal = 0;
		if rows[0][0]:
			sum_req_normal = int(rows[0][0]);
		
		sqlstr = "select SUM(error_num) from ppview.ppview_chat_%s_summary  where date = '%s' and error_id != 0;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		sum_req_err = 0;
		if rows[0][0]:
			sum_req_err = int(rows[0][0]);


		sqlstr = "select error_id,error_reason from ppview.ppview_error_reason;"
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		for row in rows:
			error_dict[row[0]] = row[1];
		print "<html><body>"
		
		print '<h1 align="center">' , "%s %s日志统计" % (kEnv,kDate) , "</h1>"
		print '</br>'
		print '<h3 align="center">请求数量占比统计</h3>'
		print '<table border="1" align="center">'
		print '<tr><th>请求总数量</th><th>正常数量</th><th>错误数量</th><th>正确占比</th><th>错误占比</th></tr>' 
		table = (sum_req, sum_req_normal, sum_req_err, "%.2lf" % (sum_req_normal * 100.0 / sum_req), "%.2lf" % (sum_req_err * 100.0 / sum_req))
		print "<tr>"
		for val in table:
			print "<td>", str(val), "</td>"
		print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">正确占比统计</h3>'
		print '<table border="1" align="center">'
		print '<tr><th>请求类型</th><th>数量</th><th>error_id</th><th>error_原因</th><th>占比</th></tr>' 
		sqlstr = "select type,sum(error_num),error_id from ppview.ppview_chat_%s_summary  where date = '%s' and error_id = 0 group by type,error_id;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_normal > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				row = row +  ( ('%.2f' % (float(row[1]) / sum_req_normal * 100)),)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">错误占比统计</h3>'
		print '<table border="1" align="center">'
		print '<tr><th>请求类型</th><th>数量</th><th>错误id</th><th>错误原因</th><th>错误占比</th></tr>' 
		sqlstr = "select type,sum(error_num),error_id from ppview.ppview_chat_%s_summary  where date = '%s' and error_id != 0 group by type,error_id;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_err > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				row = row +  ( ('%.2f' % (float(row[1]) / sum_req_err * 100)),)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		
		print '<h3 align="center">有结果超时占比统计</h3>'
		print '<table border="1" align="center">'
		print '<tr><th>请求类型</th><th>数量</th><th>错误id</th><th>错误原因</th><th>超时占比</th></tr>' 
		sqlstr = "select type,count(*),error_id from ppview.ppview_chat_%s_log where (rootTimeOut = 'true' or richTimeOut = 'true' or dfsTimeOut = 'true' or routeTimeOut = 'true') and error_id = '0' and date = '%s' group by type,error_id;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_normal > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				row = row +  ( ('%.2f' % (float(row[1]) / sum_req_normal * 100)),)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">错误超时占比统计</h3>'	
		print '<table border="1" align="center">'
		print '<tr><th>请求类型</th><th>数量</th><th>错误id</th><th>错误原因</th><th>超时占比</th></tr>' 
		sqlstr = "select type,count(*),error_id from ppview.ppview_chat_%s_log where (rootTimeOut = 'true' or richTimeOut = 'true' or dfsTimeOut = 'true' or routeTimeOut = 'true') and error_id != '0' and date = '%s' group by type,error_id;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_err > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				row = row +  ( ('%.2f' % (float(row[1]) / sum_req_err * 100)),)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">扔点情况占比统计</h3>'	
		print '<table border="1" align="center">'
		print '<tr><th>请求类型</th><th>数量</th><th>错误id</th><th>错误原因</th><th>扔点占比</th></tr>' 
		sqlstr = "select type,count(*),error_id from ppview.ppview_chat_%s_log where throwPlace=1 and error_id = '0' and date = '%s' group by type,error_id;" %(kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_normal > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("扔点warning",)
				row = row +  ( ('%.2f' % (float(row[1]) / sum_req_normal * 100)),)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">错误qid显示</h3>'	
		print '<table border="1" align="center">'
		print '<tr><th>qid</th><th>请求类型</th><th>错误id</th><th>错误原因</th></tr>' 
		sqlstr = "select qid,type,error_id from ppview.ppview_chat_%s_log where error_id!= '0' and date = '%s' limit 3;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_err > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'		
		print '</br>'
		print '<h3 align="center">有结果超时qid显示</h3>'
		print '<table border="1" align="center">'
		print '<tr><th>qid</th><th>请求类型</th><th>错误id</th><th>错误原因</th></tr>' 
		sqlstr = "select qid,type,error_id from ppview.ppview_chat_%s_log where (rootTimeOut = 'true' or richTimeOut = 'true' or dfsTimeOut = 'true' or routeTimeOut = 'true') and error_id = '0' and date = '%s' limit 3;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_normal > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">错误超时qid显示</h3>'	
		print '<table border="1" align="center">'
		print '<tr><th>qid</th><th>请求类型</th><th>错误id</th><th>错误原因</th></tr>' 
		sqlstr = "select qid,type,error_id from ppview.ppview_chat_%s_log where (rootTimeOut = 'true' or richTimeOut = 'true' or dfsTimeOut = 'true' or routeTimeOut = 'true') and error_id != '0' and date = '%s' limit 3;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_err > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("未入库error",)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print '</br>'
		print '<h3 align="center">扔点情况qid显示</h3>'	
		print '<table border="1" align="center">'
		print '<tr><th>qid</th><th>请求类型</th><th>错误id</th><th>错误原因</th></tr>' 
		sqlstr = "select qid,type,error_id from ppview.ppview_chat_%s_log where throwPlace=1 and error_id = '0' and date = '%s' and type = 's127' limit 10;" % (kEnv,kDate)
		cur.execute(sqlstr)
		conn.commit()
		rows = cur.fetchall()
		if sum_req_normal > 0:
			for row in rows:
				if row[2] in error_dict:
					row = row + (error_dict[row[2]],)
				else:
					row = row + ("扔点warning",)
				print "<tr>"
				for val in row:
					print  "<td>" ,str(val), "</td>"
				print "</tr>"
		print '</table>'
		print "<body><html>"


		cur.close()
		conn.close()
	except MySQLdb.Error,e:
		print >> sys.stderr, "Mysql Error %d: %s" % (e.args[0], e.args[1])

kDate = '20150101'
kEnv="online"

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
	if opt.env not in ("test","online"):
		print "Error Format environment"
		return
	else:
		kEnv=opt.env

	Report();

if __name__ == '__main__':
	main()

