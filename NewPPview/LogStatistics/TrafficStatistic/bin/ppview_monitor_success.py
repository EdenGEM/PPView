#!/usr/local/bin/python
#coding=utf-8

import time
import datetime
import os
import os.path
import json
import MySQLdb
import re
import optparse
import sys
sys.path.append("./bin/Common")
import Common.DBHandle

qid_dic = dict()
resp_dic = dict()
uid=0
total=0
def GetInfo():
#def GetInfo(lines):
	global qid_dic,uid
	for line in sys.stdin:
#	for line in lines:
		#qid 抓请求
		pat_log = re.compile(r'\[ERR\]\[[0-9_:]+\]\[8003STAT\] {"query":(.+),"resp":(.+)}')
#		pat_replace = re.compile(r'\[ERR\]\[[0-9_:]+\]\[8003STAT RealTimeTraffic::DoReplace] {"qid":"(\d+)","result":"(.+)"}')
		pat_replace = re.compile(r'\[ERR\]\[[0-9_:]+\]\[8003STAT\] qid:(\d+),RealTimeTraffic::DoReplace Result:(\w+)')
		match_log = pat_log.search(line)
		match_replace = pat_replace.search(line)
		logquery = ""
		logresp = ""
		result=""
		if match_log:
			logquery = match_log.group(1);
#			print"logquery=%s"%logquery
			logresp=match_log.group(2)
#			print "logresp=%s"%logresp
		if logquery=="" and not match_replace:
			continue;
		result=""
		if logquery:
			json_query=json.loads(logquery)
			qid=json_query["qid"]
			if qid not in qid_dic:
				qid_dic[qid]=dict()
				qid_dic[qid]["resp"]=list()
				qid_dic[qid]["resp"].append(logresp)
				qid_dic[qid]["totalTraf"]=0
				qid_dic[qid]["realTraf"]=0
				qid_dic[qid]["notRealTraf"]=0
			else:
				qid_dic[qid]["resp"].append(logresp)
		elif match_replace:
			traf_qid = match_replace.group(1)
			result = match_replace.group(2)
			if result == "":
				print "no result"
				continue
			if traf_qid not in qid_dic:
				qid_dic[traf_qid]=dict()
				qid_dic[traf_qid]["resp"]=list()
				qid_dic[traf_qid]["totalTraf"]=0
				qid_dic[traf_qid]["realTraf"]=0
				qid_dic[traf_qid]["notRealTraf"]=0
			if result == "Success":
				qid_dic[traf_qid]["realTraf"]+=1
			elif result == "fail":
				print "replace fail qid:",traf_qid
				qid_dic[traf_qid]["notRealTraf"]=+1
			qid_dic[traf_qid]["totalTraf"]+=1
			
def ParseInfo():
	global qid_dic
	global resp_dic
	global total
	print "in ParseInfo"
	print "qid_dic size: ",len(qid_dic)
	for qid,item in qid_dic.items():
		if len(item) == 0:
			print "there are not resp in this qid"
			continue
		resp_dic[qid]=dict()
		resp_dic[qid]["totalTraf"]=item["totalTraf"]
		resp_dic[qid]["realTraf"]=item["realTraf"]
		resp_dic[qid]["notRealTraf"]=item["notRealTraf"]
		resp_dic[qid]["taxi_sphere"]=0
		resp_dic[qid]["taxi_google"]=0
		resp_dic[qid]["taxi_cache"]=0
		resp_dic[qid]["walk_sphere"]=0
		resp_dic[qid]["walk_google"]=0
		resp_dic[qid]["walk_cache"]=0
		resp_dic[qid]["public_sphere"]=0
		resp_dic[qid]["public_google"]=0
		resp_dic[qid]["public_cache"]=0
		resp_dic[qid]["taxi_gc"]=0
		resp_dic[qid]["walk_gc"]=0
		resp_dic[qid]["public_gc"]=0
		resp_dic[qid]["google_twp"]=0
		resp_dic[qid]["cache_twp"]=0
		resp_dic[qid]["sphere_twp"]=0
		resp_dic[qid]["twp_gc"]=0
		resp_dic[qid]["total"]=0
		#遍历resp,同一个qid可能有多条结果
		for resp in item["resp"]:
			resp = json.loads(resp)
			respinfoJson=resp["data"]["info"]
			if "resp_num" not in resp:
				resp_dic[qid]["total"]+=resp["req_num"]
				total+=resp["req_num"]
			else:
				resp_dic[qid]["total"]+=resp["resp_num"]
				total+=resp["resp_num"]
			for key in respinfoJson:
				#print "respinfoJson[%s]="%key,respinfoJson[key]
				ts=0
				tg=0
				tc=0
				ws=0
				wg=0
				wc=0
				ps=0
				pg=0
				pc=0
				a = respinfoJson[key]
	#				print "a=%s"%a
				len1=len(a)
	#				print "len=%d"%len1
				for i in range(0,len1):
					resp_dic[qid]["taxi"]=0
					resp_dic[qid]["walk"]=0
					resp_dic[qid]["public"]=0
					resp_dic[qid]["sphere"]=0
					resp_dic[qid]["google"]=0
					resp_dic[qid]["cache"]=0
					b=a[i].split("|")
					mode=b[0]
	#					print "mode=%s"%mode
					if mode=="0":
						 resp_dic[qid]["taxi"] =1
					elif mode == '1':
						 resp_dic[qid]["walk"] =1
					else:
						 resp_dic[qid]["public"] =1
					#resp_dic[qid]["modenum"]+=1
					d=b[1].split("_")
					if len(d)==6:
						resp_dic[qid]["google"]=1
	#						print "len(d)=%d"%len(d)
					elif len(d)==5:
						temp0=re.compile("Sphere")
						c=d[4]
	#					print "c=%s"%c
	#						temp1=re.compile("[0-9]")
						match_mid0=temp0.search(c)
	#						match_mid1=temp1.search(c)
						if match_mid0:
							resp_dic[qid]["sphere"]=1
						else:
							resp_dic[qid]["cache"]=1
					elif len(d)==4:
						resp_dic[qid]["cache"]=1
	#					print "resp_dic[qid][google]=%s"%resp_dic[qid]["google"]
					if resp_dic[qid]["taxi"] == 1 and resp_dic[qid]["sphere"] == 1:
						ts=1
					if resp_dic[qid]["taxi"]==1 and resp_dic[qid]["google"]==1:
						tg=1
					if resp_dic[qid]["taxi"]==1 and resp_dic[qid]["cache"]==1:
						tc=1
					if resp_dic[qid]["walk"]==1 and resp_dic[qid]["sphere"]==1:
						ws=1
					if resp_dic[qid]["walk"]==1 and resp_dic[qid]["google"]==1:
						wg=1
					if resp_dic[qid]["walk"]==1 and resp_dic[qid]["cache"]==1:
						wc=1
					if resp_dic[qid]["public"]==1 and resp_dic[qid]["sphere"]==1:
						ps=1
					if resp_dic[qid]["public"]==1 and resp_dic[qid]["google"]==1:
						pg=1
					if resp_dic[qid]["public"]==1 and resp_dic[qid]["cache"]==1:
						pc=1

				if ts==1:
					resp_dic[qid]["taxi_sphere"]+=1
				if tg==1:
					resp_dic[qid]["taxi_google"]+=1
				if tc==1:
					resp_dic[qid]["taxi_cache"]+=1
				if ws==1:
					resp_dic[qid]["walk_sphere"]+=1
				if wg==1:
					resp_dic[qid]["walk_google"]+=1
				if wc==1:
					resp_dic[qid]["walk_cache"]+=1
				if ps==1:
					resp_dic[qid]["public_sphere"]+=1
				if pg==1:
					resp_dic[qid]["public_google"]+=1
				if pc==1:
					resp_dic[qid]["public_cache"]+=1
				if tg==1 or tc==1:
					resp_dic[qid]["taxi_gc"]+=1
				if wg==1 or wc==1:
					resp_dic[qid]["walk_gc"]+=1
				if pg==1 or pc==1:
					resp_dic[qid]["public_gc"]+=1
				if tg==1 or wg==1 or pg==1:
					resp_dic[qid]["google_twp"]+=1
				if tc==1 or wc==1 or pc==1:
					resp_dic[qid]["cache_twp"]+=1
				if ts==1 or ws==1 or ps==1:
					resp_dic[qid]["sphere_twp"]+=1
				if (tg==1 or tc==1) or (wg==1 or wc==1) or (pg==1 or pc==1):
					resp_dic[qid]["twp_gc"]+=1
					

def Stat():
	global resp_dic;
	print "in Stat" 
	onlinedb=Common.DBHandle.DBHandle("127.0.0.1","root","","test")
	sqlstr="replace into test.Traf8003(qid,date,8003stat) values(%s,%s,%s)"
	args=[]
	for qid,item in resp_dic.items():
#		print qid
		stat=json.dumps(item)	
#		print "stat:",stat
		T=(qid,kDate,stat)
		args.append(T)
	print "args size: ",len(args)
	onlinedb.do(sqlstr,args)	
	print "store ok"
kDate='20170815'
def main():
	global kDate
#	file1 = open("/search/log/8003/20170815_15.err","r")
#	lines = file1.readlines()
#	GetInfo(lines);
#	print "next is parseinfo~"
	parser=optparse.OptionParser()
	parser.add_option('-d', '--date',help='run date like 20150101')
	opt,args =parser.parse_args()
	try:
		run_day = datetime.datetime.fromtimestamp(time.mktime(time.strptime(opt.date, '%Y%m%d')))
		date_str = run_day.strftime('%Y%m%d')
		kDate=date_str
	except:
		print "Error Format date"
		return 
	GetInfo();
	ParseInfo();

	Stat();
	print "total:",total

if __name__ == "__main__":
	main()
