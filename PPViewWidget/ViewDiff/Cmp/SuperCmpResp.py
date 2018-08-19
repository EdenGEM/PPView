#!/usr/local/bin//python 
#-*-coding:UTF-8 -*-
import time
import datetime
import os
import os.path
import json
import MySQLdb
import re
import sys
import io
import urllib2
import optparse
import conf
import commands
import requests
import datetime
sys.path.append('../../Common')
from DBHandle import DBHandle
dbb = 'log_query_md'
NGINX_API_IP = "10.19.6.9"
TEST_PORT = "92"
OFFLINE_PORT = "91"
ONLINE_PORT = "93"
TIMEOUT = 40

reload(sys);
exec("sys.setdefaultencoding('utf-8')");
class Resp:
	def __init__(self,kList="",kIds="",kProducts="",kDiff="",kDurings="",kWholeRank="0_0.0"):
		self.kList = kList #列表里每个元素代表一个城市
		self.kIds = kIds#整个旅程中的所有景点?
		self.kProducts = kProducts#整个旅程中的所有产品
		self.kDiff = kDiff
		self.kDurings = kDurings
		self.kWholeRank = kWholeRank#评判规划效果的指标 依据景点排名

class RespCmp:
	qid_dic=dict()
	id_ranking=dict()
	AllQidIdNum1=0
	AllQidIdNum2=0
	AllQidAvg1=0.00
	AllQidAvg2=0.00
	delete=0
	view_num=0
	sameNum=0
	product_num=0
	error_num=0
	allNum = 0

	def __init__(self,kSource,kDate="",kType="",kNum="",kQid="",kCmp1="",kCmp2="",Mail="",fresh=""):
		self.kSource=kSource
		self.kDate=kDate
		self.kType=kType
		self.kQid=kQid
		self.kNum=kNum
		self.kCmp1=kCmp1
		self.kCmp2=kCmp2
		self.Mail=Mail
		self.fresh=fresh

	def LoadReqAndResp(self):
		if self.kQid != "":
			kQid=self.kQid
			tt=int(kQid)
			kDate=time.strftime("%Y%m%d",time.localtime(tt/1000))
			print "LoadReqAndRespByQid Date:",kDate
			self.kDate=kDate
		#self.__LoadRanking()
		self.__Load()
		self.__Parsereq()
		self.allNum = len(self.qid_dic)

	def CmpInfo(self):
		self.__Fetch()
		self.__GetResult()
		self.__Store()
		self.__mymail(self.qid_dic)

	def __mymail(self,Dict):
		flag=0
		if self.fresh==1:
			env1=self.kCmp1
		else:
			env1="ori"
		if self.kCmp2 not in ("test","test1","offline"):
			env2="port"
		else:
			env2=self.kCmp2
		html='''
		<html><body>
		<h1 align="center">diff %s_%s</h1>
		<table border='1' align="center">
		<tr><th>action</th><th>ori_req</th><th>%s_resp</th><th>%s_resp</th><th>diff</th></tr>
		'''%(env1,env2,env1,env2)

		for qid,item in Dict.items():
			if "difference" in item:
				flag=1
				action=-1
				if self.kType=='s227':
					req=item["ori_req"]
					pat_query=re.compile(r'&query=([^&]+)')
					match_query=pat_query.search(json.dumps(req))
					if match_query:
					   query=match_query.group(1)
					   query=urllib2.unquote(query.decode('utf-8','replace').encode('gbk','replace'))
					   tmp=json.loads(query)
					   action=tmp["action"]
					   print "action=%d"%action

				html+='''
				<tr>
					<td>%s</td>
					<td>
					<a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=ori&load=query" target="_blank">%s</a>
					</td>
					<td>
					<a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=%s&load=resp" target="_blank">%s</a>
					</td>
					<td>
					<a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=%s&load=resp" target="_blank">%s</a>
					</td>
					<td>%s</td>
				</tr>
				'''%(action,item["ori_qid"],item["ori_qid"],item["cmp1_qid"],env1,item["cmp1_qid"],item["cmp2_qid"],env2,item["cmp2_qid"],item["difference"])
		if flag==0:
			html='''
			<html><body>
			<h1 align="center">all is same</h1>
			</table>
			'''
			html+='<p align="left">AllQidNum: %d, SameNum: %d'%(self.allNum, self.sameNum)
		else:
			html+='</table>'
			html+='<p align="left">AllQidNum: %d, SameNum: %d'%(self.allNum, self.sameNum)
			html+='</p>'
			if self.allNum == 0:
				self.allNum = 1
			html+='<p align="left">ViewDiffNum: %d(%.2f'%(self.view_num,float(float(self.view_num)/(self.allNum))*100)
			html+='%)</p>'
			html+='<p align="left">productDiffNum: %d(%.2f'%(self.product_num,float(float(self.product_num)/(self.allNum))*100)
			html+='%)</p>'
			html+='<p align="left">TotalDiffNum: %d(%.2f'%(self.error_num,float(float(self.error_num)/(self.allNum))*100)
			html+='%)</p>'
			if self.kType == 's227':
				html+='<p align="left">%s_TotalViewNum_TotalAvg:%d_%.2f</p>'%(env1,self.AllQidIdNum1,float(self.AllQidAvg1)/self.AllQidIdNum1 if self.AllQidIdNum1 != 0 else 0.00)
				html+='<p align="left">%s_TotalViewNum_TotalAvg:%d_%.2f</p>'%(env2,self.AllQidIdNum2,float(self.AllQidAvg2)/self.AllQidIdNum2 if self.AllQidIdNum2 != 0 else 0.00)
		html+='</body></html>'
		with io.open('./report.html','w',encoding='utf-8') as f:
			f.write((html).decode('utf-8'))
		print "Report ok"
		Dir=commands.getoutput('pwd')
		Report=Dir+"/report.html"
#		print Report
		mailto=self.Mail
		commands.getoutput('mioji-mail -m '+mailto+' -b "CmpInfo" -f '+Report+' -h 123')
		print "mail ok"

	def __LoadRanking(self):
		id_ranking=self.id_ranking;
		conn=DBHandle(conf.Testhost,conf.Testuser,conf.Testpasswd,conf.Testdb)
		sqlstr="select id,ranking from chat_attraction;"
		resps=conn.do(sqlstr);
		if len(resps) ==0:
			print "load ranking from base_data.chat_attraction error"
			exit(1)
		for row in resps:
			id_ranking[row["id"]]=int(row["ranking"])
		print "base_data: id_ranking size:",len(id_ranking)
		
		conn=DBHandle(conf.TestPrihost,conf.TestPriuser,conf.TestPripasswd,conf.TestPridb)
		sqlstr="select real_id from attraction;"
		priResps=conn.do(sqlstr)
		if len(priResps) == 0:
			print "Load private status error"
			exit(1)
		for row in priResps:
			id_ranking[row["real_id"]]=0
		print "after add private data, id_ranking size:",len(id_ranking)
		self.id_ranking=id_ranking

	def __Load(self):
		print "in Load"
		kDate=self.kDate
		kType=self.kType
		kNum=self.kNum
		kQid=self.kQid
		qid_dic=self.qid_dic
		if self.kSource=='bug':
			conn=DBHandle(conf.Storehost,conf.Storeuser,conf.Storepasswd,conf.Storedb)
			if self.kQid != "":
				sqlstr="select qid,req,resp from error_qid where qid=%s and type='%s' and env='%s';"%(kQid,kType,self.kCmp1)
			else:
				sqlstr="select qid,req,resp from error_qid where type='%s' and env='%s' order by rand() limit %d;"%(kType,self.kCmp1,kNum)
			print sqlstr
			rows=conn.do(sqlstr)
			if len(rows)==0:
				print "mysql have no data"
				exit(1)
			for i,row in enumerate(rows):
				qid=row['qid']
				qid_dic[qid]=dict()
				qid_dic[qid]["ori_qid"]=qid
				qid_dic[qid]["ori_req"]=row['req']
				qid_dic[qid]["ori_resp"]=row['resp']
				tmp=json.loads(qid_dic[qid]["ori_resp"])
				qid_dic[qid]["ori_eid"]=tmp['error']['error_id']
		else:
			conn=DBHandle(conf.Loadhost,conf.Loaduser,conf.Loadpasswd,dbb)
			if self.kQid != "":
				sqlstr="select qid,content from log_md_%s where qid=%s and type='%s';"%(kDate,kQid,kType)
			else:
				sqlstr="select qid,content from log_md_%s where type='%s' order by rand() limit %d;"%(kDate,kType,kNum)
			print sqlstr
			qids=conn.do(sqlstr)
			if len(qids)==0:
				print "mysql have no data"
				exit(1)
			for i,row in enumerate(qids):
				qid=row["qid"]
				qid_dic[qid]=dict()
				qid_dic[qid]["ori_qid"]=qid
				qid_dic[qid]["req_type"]=kType
				print row["qid"]
				tmp = json.loads(row["content"])
				print "type of req&resp from loadhost",type(tmp["resp"]),type(tmp["req"])
				print "req from loadhost is like:",tmp["req"]
				print "resp from loadhost is like:",tmp["resp"]
				print "---------------------"
				if tmp["resp"]=="":
					continue
				try:
					qid_dic[qid]["ori_resp"] = tmp["resp"]
					qid_dic[qid]["ori_req"] = tmp["req"]
					tmp = json.loads(qid_dic[qid]["ori_req"])
					if (type(tmp) != dict):
						continue
					tmp=json.loads(qid_dic[qid]["ori_resp"])
					if (type(tmp) != dict):
						continue
				except:
					print "pass this resp"
					pop_item=qid_dic.pop(qid)
					self.delete+=1
					print "resp_loads_error,delete=%d"%(self.delete)
					continue;
				if "error" not in tmp:
					pop_item=qid_dic.pop(qid)
					self.delete+=1
					print "no error_delete=%d"%(self.delete)
					continue;
				if "error_id" in tmp["error"]:
					eid=tmp["error"]["error_id"]
				elif "errorid" in tmp["error"]:
					print "appear errorid"
					eid=tmp["error"]["errorid"]
				else:
					pop_item=qid_dic.pop(qid)
					self.delete+=1
					print "no error_id,delete=%d"%self.delete
				qid_dic[qid]["ori_eid"]=eid

			# test 库中的error_id !=0 过滤
				if eid != 0:
					pop_item=qid_dic.pop(qid)
					self.delete+=1
					print "ori_eid !=0,delete=%d"%self.delete

		self.qid_dic=qid_dic

	def __Parsereq(self):
		kQid=self.kQid
		kType = self.kType
		qid_dic=self.qid_dic
		print "in parser"
		
		for qid,item in qid_dic.items():
			if "ori_qid" not in qid_dic[qid] or "ori_req" not in qid_dic[qid] or "ori_eid" not in qid_dic[qid]:
				pop_item=qid_dic.pop(qid);
				self.delete+=1
				print "T,delete=%d"%(self.delete)
				continue;
			req=qid_dic[qid]["ori_req"]
			ori_uid=re.compile(r'&uid=([^&]+)')
			ziduan="&uid=DataPlan"
			req=re.sub(ori_uid,ziduan,req)
			print "query_req=%s"%req
			#print "ori_resp=%s"%(qid_dic[qid]["ori_resp"])
			self.__Query(qid,self.kCmp2,req)
			if self.fresh==1:
				self.__Query(qid,self.kCmp1,req)

	def __Query(self,qid,env,req):	 #拿req 打不同的服务器
		qid_dic=self.qid_dic
		env_qid="%s_qid"%env
		env_resp="%s_resp"%env
		env_req="%s_req"%env
		env_eid="%s_eid"%env
#		req = "type=" + self.kType + "&lang=zh_cn&qid=1525921604212&ptid=ptid&auth=&ccy=CNY&query="+req
		#print "in Query"
		cur_qid=int(round(time.time()*1000))
		qid_dic[qid][env_qid]=cur_qid
		#print "ori_qid=%s"%qid
		#print "%s_qid=%s"%(env,cur_qid)
		ziduan="qid="+str(cur_qid)
		originqid=r'qid=(\d+)'
		req=re.sub(originqid,ziduan,req)
	#	print "%s=%s"%(env_req,req)
		#print "self.fresh=%d"%self.fresh
		qid_dic[qid][env_req]=req
		if env=="test":
			url="http://"+NGINX_API_IP+":"+TEST_PORT+"/?"
		elif env=="online":
			url="http://"+NGINX_API_IP+":"+ONLINE_PORT+"/?"
		elif env=="offline":
			url="http://"+NGINX_API_IP+":"+OFFLINE_PORT+"/?"
		else:
			url="http://"+env+"/"
		print "%s_url=%s"%(env,url)
		url += req
		'''
		_params = {}
		_t = req.split('&')
		for i in _t:
			_tt = i.split('=')
			if _tt[0] == "qid":
				_tt[1] = str(cur_qid)
			if _tt[0] == "uid":
				_tt[1] = "dataplan"
			if _tt[0] == "query":
				_tt[1] = urllib2.unquote(_tt[1])
			if len(_tt) == 2:
				_params[str(_tt[0])]=str(_tt[1])
				print _tt[0], '  ', _tt[1]
		'''
		try:
			if(self.kType =='s225'):
				TIMEOUT = 5
			print 'xxxxxxxxxxxxxxx', url
			r=requests.get(url,timeout = TIMEOUT)
			#r=requests.get(url,params = _params,timeout = TIMEOUT)
			resps=r.text
			r.raise_for_status()
			print "get resp %s=%s"%(env_resp,resps)
			qid_dic[qid][env_resp]=resps
			print "resps's type is",type(resps)
			tmp=json.loads(resps)
			new_eid=tmp["error"]["error_id"]
			#print "%s=%s"%(env_eid,new_eid)
			qid_dic[qid][env_eid]=new_eid
		except (requests.RequestException) as e:
			print "requesterror:"
			print e
			qid_dic[qid][env_resp]=""
			qid_dic[qid][env_eid]=None

		except Exception:
			print "url request TIMEOUT~"
			qid_dic[qid][env_resp]=""
			qid_dic[qid][env_eid]=None
		self.qid_dic=qid_dic

	def __CalDuring(self,req):
			#比较checkin和checkout
			Durings = list()
			print "in CalDuring,the type of req is:",type(req)
			query=json.loads(req)
			if self.kType == "s227":
				List=query["list"]
			else:
				List=list()
				List.append(query["city"])
			listx = 0
			while listx < len(List):
				checkin=List[listx]["checkin"]
				checkout=List[listx]["checkout"]
				print "checkin:",checkin,"checkout:",checkout
				if (checkin and checkin != "") and (checkout and checkout != ""):
					kin=datetime.datetime.strptime(checkin,'%Y%m%d')
					kout=datetime.datetime.strptime(checkout,'%Y%m%d')
					delta=kout-kin
					during=delta.days+1
				else:
					if listx != 0 and listx != len(List)-1:#不过夜时checkin,checkout都为空
						during = 1
					else:
						during = 0
				Durings.append(during)
				listx+=1
			return Durings

	def __Store(self):	  #将对比环境的各自响应信息存库
		qid_dic=self.qid_dic
		kDate=self.kDate
		kType=self.kType
		conn=DBHandle(conf.Storehost,conf.Storeuser,conf.Storepasswd,conf.Storedb)
		print "before store"
		if self.fresh!=1:
			Senv='ori'
		else:
			Senv=self.kCmp1
		Denv=self.kCmp2
		Senv_qid='%s_qid'%Senv
		Senv_resp='%s_resp'%Senv
		Denv_qid='%s_qid'%Denv
		Denv_resp='%s_resp'%Denv
		if self.fresh !=1:
			if self.kCmp2 not in ("test","test1","offline"):
				sqlstr="replace into cmp_cases (date,req_type,ori_req,ori_qid,ori_resp,port_qid,port_resp)"
			else:
				sqlstr="replace into cmp_cases (date,req_type,ori_req,ori_qid,ori_resp,%s,%s)"%(Denv_qid,Denv_resp)
		else:
			if self.kCmp2 not in ("test","test1","offline"):
				sqlstr="replace into cmp_cases (date,req_type,ori_req,ori_qid,%s,%s,port_qid,port_resp)"%(Senv_qid,Senv_resp)
			else:
				sqlstr="replace into cmp_cases (date,req_type,ori_req,ori_qid,%s,%s,%s,%s)"%(Senv_qid,Senv_resp,Denv_qid,Denv_resp)
		if self.fresh !=1:
			sqlstr+=" values(%s,%s,%s,%s,%s,%s,%s)"
		else:
			sqlstr+=" values(%s,%s,%s,%s,%s,%s,%s,%s)"
		args=[]
		if self.fresh !=1:
			for qid,item in qid_dic.items():
				T=(kDate,\
						kType,\
						item["ori_req"],\
						item["ori_qid"],\
						item["cmp1_resp"],\
						item[Denv_qid],\
						item["cmp2_resp"])
				args.append(T)
		else:
			for qid,item in qid_dic.items():
				T=(kDate,\
						kType,\
						item["ori_req"],\
						item["ori_qid"],\
						item[Senv_qid],\
						item["cmp1_resp"],\
						item[Denv_qid],\
						item["cmp2_resp"])
				args.append(T)
		if len(args)==0:
			conn.do(sqlstr,None)
		else:
			conn.do(sqlstr,args)
		print "after store"

	def __Fetch(self):	#拿两个环境的response
		qid_dic=self.qid_dic
		kCmp1=self.kCmp1
		kCmp2=self.kCmp2
		kDate=self.kDate
		kType=self.kType
		kNum=self.kNum
		
		if self.fresh==0:
			env1="ori"
		else:
			env1=kCmp1
		env2=self.kCmp2

		kCmp1_qid="%s_qid"%env1
		kCmp1_resp="%s_resp"%env1
		kCmp1_eid="%s_eid"%env1
		kCmp2_qid="%s_qid"%env2
		kCmp2_resp="%s_resp"%env2
		kCmp2_eid="%s_eid"%env2
		for qid,item in qid_dic.items():
			qid_dic[qid]["cmp1_qid"]=qid_dic[qid][kCmp1_qid]
			qid_dic[qid]["cmp1_resp"]=qid_dic[qid][kCmp1_resp]
			qid_dic[qid]["cmp1_eid"]=qid_dic[qid][kCmp1_eid]
			qid_dic[qid]["cmp2_qid"]=qid_dic[qid][kCmp2_qid]
			qid_dic[qid]["cmp2_resp"]=qid_dic[qid][kCmp2_resp]
			qid_dic[qid]["cmp2_eid"]=qid_dic[qid][kCmp2_eid]
		self.qid_dic=qid_dic

	def __GetResult(self):   #预处理一下error_id 和response,并存结果difference(ByPatch)
		qid_dic=self.qid_dic
		kCmp1=self.kCmp1
		kCmp2=self.kCmp2
		
		for qid,item in qid_dic.items():
			if qid_dic[qid]["cmp1_resp"]=="" or qid_dic[qid]["cmp2_resp"]=="":
				if qid_dic[qid]["cmp1_resp"]=="":
					result = "%s: no resp "%kCmp1
				if qid_dic[qid]["cmp2_resp"]=="":
					result = "%s: no resp "%kCmp2
				qid_dic[qid]["difference"]=result
				continue;
			if qid_dic[qid]["cmp1_eid"]=="" or qid_dic[qid]["cmp2_eid"]=="":
				if qid_dic[qid]["cmp1_eid"]=="":
					result = "%s: no eid "%kCmp1
				if qid_dic[qid]["cmp2_eid"]=="":
					result += "%s: no eid"%kCmp2
				qid_dic[qid]["difference"]=result
				continue;
			print "in getresult: "
			print "cmp1_qid=%s"%qid_dic[qid]["cmp1_qid"]
			print "cmp2_qid=%s"%qid_dic[qid]["cmp2_qid"]
			print "ori_req=%s"%(qid_dic[qid]["ori_req"])
			print "%s_resp=%s"%(kCmp1,qid_dic[qid]["cmp1_resp"])
			print "%s_resp=%s"%(kCmp2,qid_dic[qid]["cmp2_resp"])
			if qid_dic[qid]["cmp1_eid"]==qid_dic[qid]["cmp2_eid"]:
				resp1 = self.__ParseResp(qid_dic[qid]["cmp1_resp"])
				resp2 = self.__ParseResp(qid_dic[qid]["cmp2_resp"])
				AvgRankResult="the same average ranking"
				if self.kType in ("s225","s227","s228","s230"):
					Durings = self.__CalDuring(qid_dic[qid]["ori_req"])
					resp1.kDurings = Durings
					resp2.kDurings = Durings
				if self.kType == "s227":
					whole_Rank1 = resp1.kWholeRank
					whole_Rank2 = resp2.kWholeRank
					num1,avg1=whole_Rank1.split('_')
					num2,avg2=whole_Rank2.split('_')
					self.AllQidIdNum1+=int(num1)
					self.AllQidIdNum2+=int(num2)
					self.AllQidAvg1+=(int(num1)*float(avg1))
					self.AllQidAvg2+=(int(num2)*float(avg2))
					if whole_Rank1 != whole_Rank2:
						AvgRankResult=", cmp1_totalIdNum_avgRank: "+whole_Rank1+", cmp2_totalIdNum_avgRank: "+whole_Rank2
						print "AvgRankResult:",AvgRankResult

				result = self.__CmpResp(resp1,resp2)
				if AvgRankResult != "the same average ranking":
					result=result+AvgRankResult
			else:
				result="error_id is diff"
			print "result=%s"%result
			if result!="all is the same":
				qid_dic[qid]["difference"]=result
			else :
				self.sameNum+=1
		self.qid_dic=qid_dic
		print "GetResult ok"

	def __ParseResp(self,tmp):
		kType = self.kType
		RespInfo = Resp()#Resp存放从resp中取出来的部分需要对比的数据结构
		resp = json.loads(tmp)#tmp存在字典里是字符串，需要loads成字典
		if "data" not in resp :
			return RespInfo
		
		if kType=="s227":
			print "in s227 view_cmp"
			if "list" not in resp["data"]:
				return RespInfo
			listx=0
			kList=list()
			while listx<len(resp["data"]["list"]):
			#listx递增表示整段旅程的每个城市，此处的while遍历所有城市取数据 组织数据
				if not resp["data"]["list"][listx]["view"]:
					kList.append("")
					listx+=1
					continue;
				else:
					View=resp["data"]["list"][listx]["view"]
					city_node=self.__ParseView(View)
					kList.append(city_node)
				listx+=1
			RespInfo.kList=kList
			WholeRank=self.__CalWholeRanking(RespInfo.kList);
			print "WholeRank:",WholeRank
			RespInfo.kWholeRank=WholeRank
		elif kType=="s225":
			print "in s225 view_cmp"
			print type(resp["data"])
			if "view" not in resp["data"]:
				return RespInfo
			kList=list()
			if not resp["data"]["view"]:
				kList.append("")
				return kList
			View=resp["data"]["view"]
			city_node=self.__ParseView(View)
			kList.append(city_node)
			RespInfo.kList=kList
		elif kType in ("s228","s230"):
			print "before s228 or s230 view_cmp"
			if "city" not in resp["data"]:
				return RespInfo
			if "view" not in resp["data"]["city"] or not resp["data"]["city"]["view"]:
				return RespInfo
			kList=list()
			if not resp["data"]["city"]["view"]:
				kList.append("")
				return kList
			View=resp["data"]["city"]["view"]
			city_node=self.__ParseView(View)
			kList.append(city_node)
			kDiff = resp["data"]["diff"]
			RespInfo.kList = kList
			RespInfo.kDiff = kDiff
		elif kType in("p201"):
			if "list" not in resp["data"]:
				return RespInfo
			Ids=list()
			i=0
			while i<len(resp["data"]["list"]):
				Id = resp["data"]["list"][i]["id"]
				Ids.append(Id)
				i+=1
			RespInfo.kIds=Ids
			return RespInfo
		elif kType in("p205"):
			if "fun" not in resp["data"] :
				return RespInfo
			kProductIds=list()
			j = 0
			while j < len(resp["data"]["fun"]):
				productId = resp["data"]["fun"][j]["pid"]
				kProductIds.append(productId)
				j+=1
			return RespInfo
		RespInfo.kProducts = __ParseProduct(resp)
		return RespInfo
	
	def __ParseView(self,View):
		print "in ParseView"
		index_of_day = 0 #索引 表示第某天
		City_node=list()
		while index_of_day < len(some_day = View["summary"]["days"][index_of_day]): #遍历某个城市的每一天
			Day=dict() #存放：某一天的日期、产品、景点信息
			Day["date"]=some_day["date"] #某天的日期
			index_of_product = 0 #索引 表示某一天的第某件产品
			day_Products=list()#存放：某一天内的产品们
			day_Pois=list()#存放：某一天内的景点们
			while index_of_product < len(some_day["product"]):
				Id = some_day["product"][index_of_product]["product_id"]
				day_Products.append(Id)
				index_of_product+=1
			index_of_pois = 0 #索引 表示某一天的某个景点
			while index_of_pois < len(some_day["pois"]):
				Id = some_day["pois"][index_of_pois]["id"]
				day_Pois.append(Id)
				index_of_pois+=1
			Day["ProductIds"]=day_Products #某天的产品们
			Day["Pois"]=day_Pois #某天的景点们
			#if self.kType == 's227':
			#	Rank=self.__CalRanking(some_day)
			#	Day["rank"]=Rank
			City_node.append(Day)#每个Day表示在城市内的每一天
			index_of_day+=1
		return City_node

	def __ParseProduct(self,resp):
		if "product" not in resp["data"]:
			return RespInfo

		all_Product = resp["data"]["product"]
		if "baoche"not in all_Product:
			return RespInfo
		if "hotel"not in all_Product:
			return RespInfo
		if "traffic"not in all_Product:
			return RespInfo
		if "zuche"not in all_Product:
			return RespInfo
		if "wanle"not in all_Product:
			return RespInfo
		Products_wanle = all_Product["wanle"]

		return Products_wanle
	def __CalRanking(self,day): #放day，不放Ids是为了在邮件中增加显示view[vidx]["ranking"]
		id_ranking=self.id_ranking
		print "day:",day
		totalRanking=0
		if "view" not in day or not day["view"]:
			return "0_0.00"
		view=day["view"]
		vidx = 0
		z=0
		while vidx < len(view):
			id = view[vidx]["id"]
			if id_ranking.has_key(id):
				view[vidx]["ranking"]=id_ranking[id]
				totalRanking+=id_ranking[id]
				print "id:",id,"rank:",id_ranking[id]
				z+=1
			else:
				print "id:",id,"has no store ranking"
			vidx+=1
		avgRanking=(float)(totalRanking)/z if z != 0 else 0.00
		idNum_arvRank=str(z)+"_"+str('%.2f'%avgRanking)
		print "idNum_arvRank:",idNum_arvRank
		return idNum_arvRank 

	def __CalWholeRanking(self,lists):
		totalNum=0
		totalAvg=0.00
		listx=0
		while listx<len(lists):
			Day=lists[listx]
			didx = 0
			while didx < len(Day):
				rank=Day[didx]["rank"]
				idNum_arvRank = rank
				num,avg=idNum_arvRank.split('_')
				totalNum+=int(num)
				totalAvg+=float(avg)*int(num)
				didx+=1
			listx+=1
		Avg="%.2f"%(totalAvg/totalNum) if totalNum !=0 else 0.00
		print "totalNum:%d, Avg:"%totalNum,Avg
		return '_'.join([str(totalNum),str(Avg)])

	def __CmpViewDate(self,env,dates,listx):
		print "in CmpViewDate"
		didx=0
		day0Date = datetime.datetime.now()
		while didx <len(dates):
			date = dates[didx]
			print "date=%s"%date
			try:
				if didx==0:
					day0Date = datetime.datetime(int(date[0:4]),int(date[4:6]),int(date[6:8]))
				didxDate = datetime.datetime(int(date[0:4]),int(date[4:6]),int(date[6:8]))
				if (didxDate-day0Date).days != didx:
					result="%s: date_list[%d]day[%d] is wrong"%(env,listx,didx)
					return result
				didx+=1
			except:
				return "%s: date error: list[%d]day[%d]"%(env,listx,didx)
		return "all is the same"


	def __CmpResp(self,Resp1,Resp2):
		print "in CmpResp"
		kType = self.kType
		kCmp1 = self.kCmp1
		kCmp2 = self.kCmp2
		result = "all is the same"
		if kType in ("s225","s227","s228","s230"):
			print "in PlanRoute cmp"
			if kType in ("s228","s230"):
				if Resp1.kDiff != Resp2.kDiff:
					result = "diff 字段"
					self.view_num+=1
					self.error_num+=1
					return result
			Durings = Resp1.kDurings
			print "Durings:",Durings
			kList1 = Resp1.kList
			kList2 = Resp2.kList
			print "len kList:",len(kList1)
			print "kList1:",kList1
			if not kList1 or not kList2:
				if not Resp1.kList:
					result = "%s: list is None "%kCmp1
				if not Resp2.kList:
					result += "%s: list is None"%kCmp2
				self.view_num+=1
				self.error_num+=1
				return result
			if len(kList1) != len(kList2):
				result = "length List"
				self.view_num+=1
				self.error_num+=1
				return result
			index_of_city= 0#表示第某个城市的索引
			while index_of_city < len(kList1): #遍历整个旅程(kList)的每一个城市
				city_of_cmp1 = kList1[index_of_city] #city_of_cmp1指第一个对比对象的第index_of_city个城市的信息
				city_of_cmp2 = kList2[index_of_city] #city_of_cmp2指第二个对比对象的第index_of_city个城市的信息
				if not city_of_cmp1 or not city_of_cmp2:
					if not city_of_cmp1 and not city_of_cmp2:
						listx+=1
						continue
					elif not city_of_cmp1:
						result = "%s: list[%d] is None "%(kCmp1,index_of_city)
					elif not city_of_cmp2:
						result = "%s: list[%d] is None "%(kCmp2,index_of_city)
					self.view_num+=1
					self.error_num+=1
					return result
				print "days1:",city_of_cmp1
				print "days2:",city_of_cmp2
				if len(city_of_cmp1) != len(city_of_cmp2):
					result = "list[%d]: length_day"%(index_of_city)
					self.view_num+=1
					self.error_num+=1
					return result
				Dates1=list() #存放：在某个城市的每一天的日期
				Dates2=list()
				index_of_day = 0 #索引第某个城市的第某天
				while index_of_day < len(city_of_cmp1): #遍历这个城市的每一天
					day_of_cmp1=city_of_cmp1[index_of_day]
					day_of_cmp2=city_of_cmp2[index_of_day]
					Date1 = day_of_cmp1["date"] #在某个城市的某一天的日期
					Date2 = day_of_cmp2["date"]
					ProIds1 = day_of_cmp1["ProductIds"] #存放：某城市某天的所有产品信息
					ProIds2 = day_of_cmp2["ProductIds"]
					Pois1 = day_of_cmp1["Pois"] #存放：某城市某天的所有景点
					Pois2 = day_of_cmp2["Pois"]
					if Date1 != Date2:
						result = "list[%d]day[%d]: date"%(index_of_city,index_of_day)
						self.view_num+=1
						self.error_num+=1
						return result
					Dates1.append(Date1)
					Dates2.append(Date2)
					if kType == "s227":
						rank1 = day1["rank"]
						rank2 = day2["rank"]
						if rank1 != rank2:
							result = "list[%d]day[%d]: rank"%(index_of_city,index_of_day)
							self.view_num+=1
							self.error_num+=1
							return result
					if len(ProIds1) != len(ProIds2):
						result = "list[%d]day[%d]: length_id"%(index_of_city,index_of_day)
						self.view_num+=1
						self.error_num+=1
						return result
					index_of_product = 0 #某城市某天所有产品中的第几个产品的索引
					while index_of_product < len(ProIds1): #遍历所有产品比较
						id1 = ProIds1[index_of_product]
						id2 = ProIds2[index_of_product]
						if id1 != id2:
							result = "list[%d]day[%d]id[%d]"%(index_of_city,index_of_day,index_of_product)
							self.view_num+=1
							self.error_num+=1
							return result
						index_of_product+=1
					index_of_pois = 0 #某城市某天所有景点中的第几个景点的索引
					while index_of_pois < len(Pois1): #遍历所有景点比较
						pois1 = Pois1[index_of_pois]
						pois2 = Pois2[index_of_pois]
						if pois1 != pois2:
							self.view_num+=1
							result = "list[%d]day[%d]dur[%d]"%(index_of_city,index_of_day,index_of_pois)
							self.error_num+=1
							return result
						index_of_pois+=1
					index_of_day+=1

				if len(Dates1) != Durings[index_of_city]:
					result="%s: list[%d] checkin_checkout error "%(kCmp1,index_of_city)
					self.view_num+=1
					self.error_num+=1
					return result
				if len(Dates2) != Durings[index_of_city]:
					result="%s: list[%d] checkin_checkout error"%(kCmp2,index_of_city)
					self.view_num+=1
					self.error_num+=1
					return result
				result=self.__CmpViewDate(kCmp1,Dates1,index_of_city)
				print "cmp1ViewDate ok"
				if result=="all is the same":
					result=self.__CmpViewDate(kCmp2,Dates2,index_of_city)
				else:
					tmp=self.__CmpViewDate(kCmp2,Dates2,index_of_city)
					if tmp!="all is the same":
						result=result+', '+tmp
				print "CmpViewDate ok"
				index_of_city+=1
			
			Products1 = Resp1.kProducts
			Products2 = Resp2.kProducts
			if len(Products1) != len(Products2):
				result = "len of kProducts is not matching"
				return result
			if Products1 != Products2:
				result = "kProducts is not matching"
				return result

		elif kType in ("p201"):
			Ids1 = Resp1.kIds
			Ids2 = Resp2.kIds
			print "Ids1:",Ids1
			print "Ids2:",Ids2
			if len(Ids1) != len(Ids2):
				result = "length_List"
				self.view_num+=1
				self.error_num+=1
				return result
			idx = 0
			while idx < len(Ids1):
				id1 = Ids1[idx]
				id2 = Ids2[idx]
				if id1 != id2:
					result = "id[%d]"%idx
					self.view_num+=1
					self.error_num+=1
					return result
				idx+=1
		########################待改
		elif kType == "p205":
			ProductIds1 = Resp1.kProductIds
			ProductIds2 = Resp2.kProductIds
			print "ProductIds1:",ProductIds1
			print "ProductIds2:",ProductIds2
			if len(ProductIds1) != len(ProductIds2):
				result = "length_ProductIds"
				self.product_num+=1
				self.error_num+=1
				return result
			pdx = 0
			while pdx < len(ProductIds1):
				productId1 = ProductIds1[pdx]
				productId2 = ProductIds2[pdx]
				if productId1 != productId2:
					result = "productId[%d]"%pdx
					self.product_num+=1
					self.error_num+=1
					return result
		return result
