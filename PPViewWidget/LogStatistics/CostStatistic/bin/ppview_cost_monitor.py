#!/usr/bin/env python
#coding=utf-8

import re
import json
import sys
import MySQLdb
import LoadConfig as Conf
import PrintInfo
import time

reload(sys);
exec("sys.setdefaultencoding('utf-8')");

k_log_cost_dict = dict()
k_date = '20161111'
k_cost_log_pat = re.compile(r'^\[LOG\]\[([0-9:_]+)\]\[([0-9a-zA-Z_]+)--[0-9a-zA-Z]{0,}\]\[STATS\]query-type=([a-zA-z0-9]+),cost=([0-9]+), query=({.+}),,,ret=({.+})')
#k_cost_log_pat = re.compile(r'^\[LOG\]\[[0-9:_]+\]\[([0-9a-zA-Z_]+)--[0-9a-zA-Z]{0,}\]\[STATS\]query-type=([a-zA-z0-9]+),cost=([0-9]+)')
k_recv_log_pat = re.compile(r'^\[LOG\]\[([0-9:_]+)\]\[([0-9a-zA-Z_]+)--[0-9a-zA-Z]{0,}\]QueryProcessor::ParseWorker')

def GetInfo():
	global k_log_cost_dict
	for line in sys.stdin:
		cost_log_match = k_cost_log_pat.search(line);
		if cost_log_match:
			time_str = cost_log_match.group(1)
			qid = cost_log_match.group(2)
			q_type = cost_log_match.group(3)
			cost = int(cost_log_match.group(4))
			query = cost_log_match.group(5)
			resp = cost_log_match.group(6)
			log_dict = dict();
			log_dict["type"] = q_type
			log_dict["cost"] = cost
			time_log = time.mktime(time.strptime("2017" + time_str,'%Y%m%d_%H:%M:%S'))
			log_dict["time_log_end"] = float(time_log * 1000);
#			log_dict["time_resp"] = float("%s" % (qid))
#			print >> sys.stderr," type ",log_dict["type"] , " time sub ", log_dict["time_log"] - log_dict["time_resp"]
#			if log_dict["time_log"] - log_dict["time_resp"] < 0:
#				print >> sys.stderr, "ex1001   ",  qid, " ---> ", time_str
			try:
				log_dict["query"] = json.loads(query)
				log_dict["resp"] = json.loads(resp)
				if qid not in k_log_cost_dict:
					k_log_cost_dict[qid] = dict(log_dict)
				else:
					if "time_log_end" not in k_log_cost_dict[qid] or log_dict["time_log_end"] > k_log_cost_dict[qid]["time_log_end"]:
						k_log_cost_dict[qid].update(log_dict)
			except Exception, e:
				pass;
		recv_log_match = k_recv_log_pat.search(line);
		if recv_log_match:
			time_str = recv_log_match.group(1)
			qid = recv_log_match.group(2)
			time_log = time.mktime(time.strptime("2017" + time_str,'%Y%m%d_%H:%M:%S'))
			log_dict = dict();
			log_dict["time_log_start"] = float(time_log * 1000);
			try:
				if qid not in k_log_cost_dict:
					k_log_cost_dict[qid] = dict(log_dict)
				else:
					if "time_log_start" not in k_log_cost_dict[qid] or log_dict["time_log_start"] < k_log_cost_dict[qid]["time_log_start"]:
						k_log_cost_dict[qid].update(log_dict)
			except Exception, e:
				pass;

#			print json.dumps(k_log_cost_dict[qid]);

def WriteInfo():
	conn = None
	cur = None
	try:
		conn = MySQLdb.connect(host=Conf.k_dic_config['db_host'],user=Conf.k_dic_config['db_user'],passwd=Conf.k_dic_config['db_passwd'],db=Conf.k_dic_config['db_basedata'],port=3306,charset="utf8")
		cur=conn.cursor()
		for qid in k_log_cost_dict:
			if "time_log_end" in k_log_cost_dict[qid] and "time_log_start" in k_log_cost_dict[qid]:
				print >> sys.stderr , "type", k_log_cost_dict[qid]["type"]," time sub " ,k_log_cost_dict[qid]["time_log_end"] - k_log_cost_dict[qid]["time_log_start"] 
			else:
				continue;
#			sql = 'replace into %s (type,date,error_id,tot,qid) values (' % Conf.k_dic_config['table_cost'];
			sql = 'replace into %s (type,date,error_id,tot,qid) values (' % Conf.k_dic_config[k_table];
			sql += '\'' + k_log_cost_dict[qid]["type"] + '\','
			sql += '\'' + k_date + '\','
			sql += '\'' + str(k_log_cost_dict[qid]["resp"]["error"]["error_id"]) + '\','
			sql += '\'' + str((k_log_cost_dict[qid]["time_log_end"] - k_log_cost_dict[qid]["time_log_start"]) * 1000 ) + '\','
			sql += '\'' + qid + '\''
			sql += ');'
			#print sql
			cur.execute(sql)
			conn.commit()
	except MySQLdb.Error,e:
		 print >> sys.stderr, "Mysql Error %d: %s" % (e.args[0], e.args[1])
	finally:
		if conn:
			conn.close()
		if cur:
			cur.close()
		
def WriteHtml():
	
	conn = None
	cur = None
	try:
		conn = MySQLdb.connect(host=Conf.k_dic_config['db_host'],user=Conf.k_dic_config['db_user'],passwd=Conf.k_dic_config['db_passwd'],db=Conf.k_dic_config['db_basedata'],port=3306,charset="utf8")
		cur=conn.cursor()
		#统计
		sql = "select type,error_id, count(*) from %s where date = '%s' and error_id = 0 group by type,error_id;" % (Conf.k_dic_config[k_table], k_date)
		cur.execute(sql)
		conn.commit()
		rows = cur.fetchall()
		cost_dic = dict()
		for row in rows:
			cost_dic[row[0]] = dict()
			cost_dic[row[0]]["total"] = list()
			cost_dic[row[0]]["total"].append(str(row[2]))
			cost_dic[row[0]]["type"] = row[0];
			cost_dic[row[0]]["error_id"] = row[1];
#		WriteTable([u'类型',u'错误id',u'数量'], rows);
		#平均值
		sql = "select type,error_id, round(avg(tot) / 1000000, 2) from %s where date = '%s' and error_id = '0' group by type,error_id;" % (Conf.k_dic_config[k_table], k_date)
		cur.execute(sql)
		conn.commit()
		rows = cur.fetchall()
		for row in rows:
			cost_dic[row[0]]["total"].append(str(row[2]))
#		WriteTable([u'类型',u'错误id',u'平均耗时(s)'], rows);
		#最大值
		sql = "select type,error_id, round(max(tot) / 1000000, 2),qid from(select * from %s where date = '%s' and error_id = '0' order by tot desc) as b group by type;" % (Conf.k_dic_config[k_table], k_date)
		cur.execute(sql)
		conn.commit()
		rows = cur.fetchall()
#		WriteTable([u'类型',u'错误id',u'最大耗时(s)'], rows);
		for row in rows:
			cost_dic[row[0]]["total"].append(row[2])
			cost_dic[row[0]]["total"].append(row[3])
		#最小值
		sql = "select type,error_id, round(min(tot) / 1000000, 2) from  %s where date = '%s' and error_id = '0' group by type;" % (Conf.k_dic_config[k_table], k_date)
		cur.execute(sql)
		conn.commit()
		rows = cur.fetchall()
#		WriteTable([u'类型',u'错误id',u'最小耗时(s)`'], rows);
		for row in rows:
			cost_dic[row[0]]["total"].append(row[2])

		sql = "select type, error_id, count(*) from %s where date = '%s' and error_id = '0' group by type , error_id;" % (Conf.k_dic_config[k_table], k_date)
		cur.execute(sql)
		conn.commit()
		rows = cur.fetchall()
		for i in range(0, len(rows)):
			count = rows[i][2];
			percent_list = [50, 90];
			for j in range(0, len(percent_list)):
				sql = "select type, error_id, round(tot / 1000000, 2) from %s where type = '%s' and date = '%s' and error_id = '0' order by tot limit %d, %d" % (Conf.k_dic_config[k_table], rows[i][0], k_date, int(count * percent_list[j] * 1.0 / 100), 1);
#				sql = "select type, error_id, tot from %s where type = '%s' and error_id = '0' limit %d, %d" % (Conf.k_dic_config['table_cost'], rows[i][0], int(count * percent_list[j] * 1.0 / 100), int(count * percent_list[j] * 1.0 / 100));
				cur.execute(sql)
				conn.commit()
				limit_row = cur.fetchall()
				for row in limit_row:
					cost_dic[row[0]]["total"].append(row[2])
#				WriteTable([ 'qid',u'类型',u'错误id',u'耗时%d%%' % (percent_list[j])], limit_row);
#		for key in cost_dic:
		WriteTable([ u'类型',u'错误id',u'数量', u'平均耗时(s)', u'最大耗时(s)(qid)', u'最小耗时(s)', u'50%耗时(s)', u'90%耗时(s)'], [[cost_dic[key]["type"], cost_dic[key]["error_id"]]  + [str(iii) for iii in cost_dic[key]["total"]] for key in cost_dic ]);
	except MySQLdb.Error,e:
		 print >> sys.stderr, "Mysql Error %d: %s" % (e.args[0], e.args[1])
	finally:
		if conn:
			conn.close()
		if cur:
			cur.close()

def WriteTable(head_list, row):
#	if type(head_list) is not list or type(row) is not list:
#		PrintInfo.PrintWarning("not a avail list");
#	return 1;
	print '<table border="1" align="center">'
	print "</tr><th>" + ('</th><th>'.join(head_list)).encode('utf8') + '</th></tr>'
	for i in range(0,len(row)):
		print "<tr>"
		for j,jj in enumerate(row[i]):
			if j == 4:
				print "<td>"+str(jj)
			elif j == 5:
				print "("+str(jj)+")</td>"
			else:
				print '<td>'+str(jj) + '</td>'
		print '</tr>'
	print "</table>"

def main():
	global k_date,k_table

	if len(sys.argv) < 3:
		PrintInfo.PrintErr("need date,env ")
		exit(1)
	k_date = sys.argv[1]
        k_table =sys.argv[2]+"_cost_table"
	
	Conf.LoadConfig("./conf/config")

	GetInfo()

	WriteInfo()

	WriteHtml()

if __name__ == "__main__":
	main()
