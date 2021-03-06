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

reload(sys)
sys.setdefaultencoding('utf8')

qid_dic = dict()

def GetInfo():
	global qid_dic
	for line in sys.stdin:
#	for line in lines:
		#qid 抓请求
		pat_qid = re.compile(r'\[LOG\]\[[0-9_:]+\]\[([0-9]+)--[wc0-9]+\]QueryProcessor::ParseWorker, Recv Query')
		match_qid = pat_qid.search(line)
		if match_qid:
#			print match_qid.group(1)
			qidQid = match_qid.group(1);

			if qidQid not in qid_dic:
				qid_dic[qidQid] = dict();
				qid_dic[qidQid]["qid"] = qidQid;
		#qid 抓logDump
		pat_LogDump = re.compile(r'\[LOG\]\[[0-9_:]+\]\[([0-9]+)--[wc0-9]+\]LogDump::Dump, log=(.+)')
		match_logDump = pat_LogDump.search(line)
		if match_logDump:
#			print match_logDump.group(1),"  ", match_logDump.group(2);
			logDumpQid = match_logDump.group(1)
			logDumpJson = match_logDump.group(2)
			
			if logDumpQid not in qid_dic:
				qid_dic[logDumpQid] = dict();
				qid_dic[logDumpQid]["qid"] = logDumpQid
#				continue
			qid_dic[logDumpQid]["logJson"] = logDumpJson;
#		pat_errDump = re.compile(r'\[LOG\]\[[0-9_:]+\]\[([0-9]+)--[w0-9]+\]ErrDump::Dump, errID=([0-9]+),errReason=([.+]),querystring')
		pat_qProc = re.compile(r'\[LOG\]\[[0-9_:]+\]\[([0-9]+)--[wc0-9]+\]MQQueryProcessor::svc, [A-Za-z0-9=, ]+querystring @([^@]+)@,worker->result=(.+),Owner=OP')
		match_Qproc = pat_qProc.search(line);
		if match_Qproc:
#			print match_Qproc.group(1)
#			print match_Qproc.group(2)
#			print match_Qproc.group(3)
			qPorcQid = match_Qproc.group(1)
			qProcReq = match_Qproc.group(2)
			qProcResp = match_Qproc.group(3)

			if qPorcQid not in qid_dic:
				qid_dic[qPorcQid] = dict();
#				qid_dic[qPorcQid]["qid"] = dict();
				qid_dic[qPorcQid]["qid"] = qPorcQid
#				print qPorcQid;
#				continue
			qid_dic[qPorcQid]["req"] = qProcReq
			qid_dic[qPorcQid]["resp"] = qProcResp
			qid=qPorcQid
			pat_type = re.compile('type=([^&]+)');
			match_type = pat_type.search(qid_dic[qid]["req"]);
			if match_type:
	#			print match_type.group(1);
				reqType = match_type.group(1)

				qid_dic[qid]["type"] = reqType;
			respJson = json.loads(qid_dic[qid]["resp"]);
			if "throwPlace" not in qid_dic[qid]:
				qid_dic[qid]["throwPlace"]=0
#			print "qid:",qid_dic[qid]["qid"]
#			print "respJson:",qid_dic[qid]["resp"]
			if qid_dic[qid]["throwPlace"] == 0:
				if qid_dic[qid]["type"] == 's127' :
					try:
						List=respJson["data"]["list"];
					except:
						print "no data/list "
						continue;
					flag=0
					for i in range(len(List)):
						try:
							warning=List[i]["view"]["summary"]["warning"]
						except:
							print "no view/summary/warning"
							continue;
						for j in range(len(warning)):
							try:
								desc=warning[j]["desc"]
								desc=str(desc)
								pat_desc=re.compile(r'已为您尽量安排必去地点和玩乐')
								match_desc=pat_desc.search(desc)
								if match_desc:
									qid_dic[qid]["throwPlace"]=1
									flag=1
									break;
							except:
								print "desc error"
						if flag==1:
							break;
				if qid_dic[qid]["type"] == 's130':
					try:
						warning=respJson["data"]["warning"]
					except:
						print "no data/warning"
						continue;
					for i in range(len(warning)):
						try:
							title=warning[i]["title"]
							title=str(title)
							pat_title=re.compile(r'由于时间安排过于紧张，需要删除以下地点')
							match_title=pat_title.search(title)
							if match_title:
								qid_dic[qid]["throwPlace"]=1
								break;
						except:
							print "title error"


def ParseInfo():
	global qid_dic
	for qid,item in qid_dic.items():
		if "qid" not in qid_dic[qid] or  "logJson" not in qid_dic[qid] or "req" not in qid_dic[qid] or "resp" not in qid_dic[qid]:
			pop_item = qid_dic.pop(qid);
#			print "message lost ", qid;
			continue
		pat_uid = re.compile('uid=([^&]+)')	
		match_uid = pat_uid.search(qid_dic[qid]["req"]);
		qid_dic[qid]["uid"]=""
		if match_uid:
#			print match_uid.group(1)
			reqUid = match_uid.group(1)
			qid_dic[qid]["uid"] = reqUid
		pat_query = re.compile(r'query=([^&]+)')
		match_query = pat_query.search(qid_dic[qid]["req"])
		if match_query:
#			print match_query.group(1);
			reqQuery = match_query.group(1)

			qid_dic[qid]["query"] = reqQuery;
		respJson = json.loads(qid_dic[qid]["resp"]);
		qid_dic[qid]["err"] = respJson["error"];

#		if qid_dic[qid]["err"]["error_id"] != 0:
#			print qid_dic[qid]["err"]["error_id"];
#			print qid_dic[qid]["err"]["error_reason"]


#		for key in qid_dic[qid]:
#			print "\t\t\t %s" % (key);
#			print qid_dic[qid][key];

stat_dict=dict()
cost_dict=dict()

def Stat():
	global stat_dict, cost_dict,kEnv;
	cmd = "REPLACE into ppview.ppview_chat_%s_log (qid ,uid, type, error_id, date, `8002`, `8003`, `8004`, dataCheck, root, rich, dfs, route, perfect ,tot, restNum, shopNum, viewNum, dfsNum, rootNum, richNum, rootTimeOut, richTimeOut, dfsTimeOut, routeTimeOut, hasTimeOut,throwPlace) values ("%kEnv
	
	stat_dict["sum"] = dict();
	stat_dict["sum"]["all"] = 0;
	stat_dict["sum"]["err"] = 0;
	stat_dict["sum"]["normal"] = 0;

	stat_dict["type"] = dict()
	for qid,item in qid_dic.items():
#对error_id	 统计~	
		if qid_dic[qid]["err"]["error_id"] not in stat_dict["type"]:
			stat_dict["type"][qid_dic[qid]["err"]["error_id"]] = 1;
		else:
			stat_dict["type"][qid_dic[qid]["err"]["error_id"]] += 1;
		stat_dict["type"][str(qid_dic[qid]["err"]["error_id"]) + "_reason"] = qid_dic[qid]["err"]["error_reason"];
#对error_id 统计汇总数目
		if qid_dic[qid]["err"]["error_id"] == 0:
			stat_dict["sum"]["normal"] += 1;
		else:
			stat_dict["sum"]["err"] += 1;
		
		stat_dict["sum"]["all"] += 1;
		sqlstr =  cmd + GetSql(item)
		conn=MySQLdb.connect(host='10.10.151.68',user='reader',passwd='miaoji1109',db='ppview',port=3306,charset="utf8")
		cur=conn.cursor()
		try:
			cur.execute(sqlstr)
			conn.commit()
			cur.close()
			conn.close()
		except MySQLdb.Error,e:
			 print >> sys.stderr, "Mysql Error %d: %s" % (e.args[0], e.args[1])
		print "stat over"
#		logJson = json.loads(qid_dic[qid]["logJson"])
		
#		if "cost" in logJson:
#			if "bagSearch" in logJson["cost"]:
				
#				print logJson["cost"]["bagSearch"]
#			else:
#				print "other"
#		print logJson
#		


#分析log专用		
#		if qid_dic[qid]["err"]["error_id"] == 55402 or qid_dic[qid]["err"]["error_id"] == 53102:
#			print qid_dic[qid]["req"]

#	for key in stat_dict["sum"]:
#		print key
#		print stat_dict["sum"][key]
#	print ""	
#	for key in stat_dict["type"]:
#		print key
#		print stat_dict["type"][key]

#	for qid,item in qid_dic.items():
#		if 


def GetSql(item):
	if item["throwPlace"]==1:
		print "throwPlace ..."
	global kDate
	insql = "'";
	insql += item["qid"] + "', '";
	insql += item["uid"] + "', '";
	insql += item["type"] + "', '";
#	insql += item["date"] + "', '";
	insql += str(item["err"]["error_id"]) + "','";
	insql += kDate + "', ";
	logJson = json.loads(item["logJson"]);
	insql += str(logJson["cost"]["traffic"]["8002"]) + ", " 
	insql += str(logJson["cost"]["traffic"]["8003"]) + ", "
	insql += str(logJson["cost"]["traffic"]["8004"]) + ", "
#	insql += str(logJson["cost"]["traffic"]["8005"]) + ", "
	insql += str(logJson["cost"]["dataCheck"]["tot"]) + ", "
	if "bagSearch" in logJson["cost"]:
		insql += str(logJson["cost"]["bagSearch"]["root"]) + ", " 
		insql += str(logJson["cost"]["bagSearch"]["rich"]) + ", " 
		insql += str(logJson["cost"]["bagSearch"]["dfs"]) + ", " 
		insql += str(logJson["cost"]["bagSearch"]["route"]) + ", "
#		if "perfect" in logJson["cost"]["bagSearch"]:
		insql += str(logJson["cost"]["bagSearch"]["perfect"]) + ", "
#		else:
#			insql += str(0) + ", "
#			print logJson

#		insql += str(logJson["cost"]["bagSearch"]["prefect"]) + ", " 
		insql += str(logJson["cost"]["bagSearch"]["tot"]) + ", " 
	else:
		insql += str(0) + ", " 
		insql += str(0) + ", " 
		insql += str(0) + ", " 
		insql += str(0) + ", " 
		insql += str(0) + ", " 
		insql += str(0) + ", "
	if "restNum" in logJson["cost"]:
		insql += str(logJson["cost"]["restNum"]) + ", "
	else:
		insql += str(0) + ", "

	if "shopNum" in logJson["cost"]:
		insql += str(logJson["cost"]["shopNum"]) + ", "
	else:
		insql += str(0) + ", "
		
	if "viewNum" in logJson["cost"]:
		insql += str(logJson["cost"]["viewNum"]) + ", "
	else:
		insql += str(0) + ", "
	
	if "bag" in logJson["stat"]:
		insql += str(logJson["stat"]["bag"]["dfsNum"]) + ", " 
		insql += str(logJson["stat"]["bag"]["rootNum"]) + ", " 
		insql += str(logJson["stat"]["bag"]["richNum"]) + ", '" 
		if "rootTimeOut" in logJson["stat"]["bag"]:
			insql += str(logJson["stat"]["bag"]["rootTimeOut"]) + "', '" 
			insql += str(logJson["stat"]["bag"]["richTimeOut"]) + "', '" 
			insql += str(logJson["stat"]["bag"]["dfsTimeOut"]) + "', '" 
			insql += str(logJson["stat"]["bag"]["routeTimeOut"]) + "','"
			if logJson["stat"]["bag"]["rootTimeOut"] == 'true' or \
				logJson["stat"]["bag"]["richTimeOut"] == 'true' or \
				logJson["stat"]["bag"]["dfsTimeOut"] == 'true' or \
				logJson["stat"]["bag"]["routeTimeOut"] == 'true':
				insql += str("true") + "', '"
			else:
				insql += str("false") + "', '"
		else:
			insql += str("NULL") + "', '" 
			insql += str("NULL") + "', '" 
			insql += str("NULL") + "', '" 
			insql += str("NULL") + "', '" 
			insql += str("NULL") + "', '"
	else:
		insql += str(0) + ", " 
		insql += str(0) + ", " 
		insql += str(0) + ", '" 
		insql += str("NULL") + "', '" 
		insql += str("NULL") + "', '" 
		insql += str("NULL") + "', '" 
		insql += str("NULL") + "', '" 
		insql += str("NULL") + "', '"

	insql += str(item["throwPlace"]) + "'"
		

	insql += ")"
	return insql


kDate = '20171122'
kEnv = 'online'

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
#	file1=open('20171122_14.err','r')
#	lines=file1.readlines()
	GetInfo();

	ParseInfo();

	Stat();

if __name__ == "__main__":
	main()
