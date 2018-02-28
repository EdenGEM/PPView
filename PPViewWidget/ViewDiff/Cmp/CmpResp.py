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

reload(sys);
exec("sys.setdefaultencoding('utf-8')");

class RespCmp:
    qid_dic=dict()
    id_ranking=dict()
    AllQidIdNum1=0
    AllQidIdNum2=0
    AllQidAvg1=0.00
    AllQidAvg2=0.00
    delete=0
    view_num=0
    product_num=0
    error_num=0

    def __init__(self,kSource,kDate="",kType="",kNum="",kQid="",kCmp1="",kCmp2="",Mail="",fresh=""):
        self.kSource=kSource
        self.kDate=kDate
        self.kType=kType
        self.kNum=kNum
        self.kQid=kQid
        self.kCmp1=kCmp1
        self.kCmp2=kCmp2
        self.Mail=Mail
        self.fresh=fresh
    def LoadReqAndRespByPatch(self):
        self.__LoadRanking()
        self.__Load()
        self.__Parsereq()
        print "LoadReqAndRespByPatch ok"

    def LoadReqAndRespByQid(self):
        kQid=self.kQid
        tt=int(kQid)
        kDate=time.strftime("%Y%m%d",time.localtime(tt/1000))
        print kDate
        self.kDate=kDate
        self.__LoadRanking()
        self.__Load()
        self.__Parsereq()
        print "LoadReqAndRespByQid ok"

    def CmpInfoByPatch(self):
        self.__Fetch()
        self.__GetResp()
        self.__Store()
        self.__mymail(self.qid_dic)

    def __mymail(self,Dict):
        html='''
        <html><body>
        <h1 align="center">diff %s_%s</h1>
        <table border='1' align="center">
        <tr><th>action</th><th>origin_query</th><th>origin_req</th><th>%s_qid</th><th>%s_qid</th><th>diff</th></tr>
        '''%(self.kCmp1,self.kCmp2,self.kCmp1,self.kCmp2)
        flag=0
        if self.fresh==1:
            env1=self.kCmp1
        else:
            env1="ori"
        if self.kCmp2 not in ("test","test1","offline"):
            env2="port"
        else:
            env2=self.kCmp2

        for qid,item in Dict.items():
            if "difference" in item:
                flag=1
                if self.kType=='s127':
                    req=item["ori_req"]
                    pat_query=re.compile(r'&query=([^&]+)')
                    match_query=pat_query.search(req)
                    if match_query:
                       query=match_query.group(1)
                       query=urllib2.unquote(query.decode('utf-8','replace').encode('gbk','replace'))
                       tmp=json.loads(query)
                       action=tmp["action"]
                       print "action=%d"%action
                else:
                    action=-1

                html+='''
                <tr>
                    <td>%s</td>
                    <td>
                    <a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=ori&load=query" target="_blank">%s</a>
                    </td>
                    <td>
                    <a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=ori&load=req" target="_blank">%s</a>
                    </td>
                    <td>
                    <a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=%s&load=resp" target="_blank">%s</a>
                    </td>
                    <td>
                    <a href="http://10.10.191.51:8000/cgi-bin/index.py?qid=%s&env=%s&load=resp" target="_blank">%s</a>
                    </td>
                    <td>%s</td>
                </tr>
                '''%(action,qid,qid,qid,qid,item["cmp1_qid"],env1,item["cmp1_qid"],item["cmp2_qid"],env2,item["cmp2_qid"],item["difference"])
        if flag==0:
            html='''
            <html><body>
            <h1 align="center">No data</h1>
            </body></html>
            '''
        else:
            html+='</table>'
            html+='<p align="left">ViewDiffNum: %d(%.2f'%(self.view_num,float(float(self.view_num)/self.kNum)*100)
            html+='%)</p>'
            html+='<p align="left">productDiffNum: %d(%.2f'%(self.product_num,float(float(self.product_num)/self.kNum)*100)
            html+='%)</p>'
            html+='<p align="left">TotalDiffNum: %d(%.2f'%(self.error_num,float(float(self.error_num)/self.kNum)*100)
            html+='%)</p>'
            if self.kType == 's127':
                html+='<p align="left">%s_TotalViewNum_TotalAvg:%d_%.2f</p>'%(env1,self.AllQidIdNum1,float(self.AllQidAvg1)/self.AllQidIdNum1 if self.AllQidIdNum1 != 0 else 0.00)
                html+='<p align="left">%s_TotalViewNum_TotalAvg:%d_%.2f</p>'%(env2,self.AllQidIdNum2,float(self.AllQidAvg2)/self.AllQidIdNum2 if self.AllQidIdNum2 != 0 else 0.00)
            html+='</body></html>'
        with io.open('./report.html','w',encoding='utf-8') as f:
            f.write((html).decode('utf-8'))
        print "Report ok"
        Dir=commands.getoutput('pwd')
        Report=Dir+"/report.html"
#        print Report
        mailto=self.Mail
        commands.getoutput('mioji-mail -m '+mailto+' -b "CmpInfo" -f '+Report+' -h 123')
        print "mail ok"

    def CmpInfoByQid(self):
        qid_dic=self.qid_dic
        kCmp1=self.kCmp1
        kCmp2=self.kCmp2
        qid=self.kQid
        if self.fresh==1:
            Senv_resp="%s_resp"%kCmp1
            Senv_eid="%s_eid"%kCmp1
            Senv_qid="%s_qid"%kCmp1
        else:
            Senv_resp="ori_resp"
            Senv_eid="ori_eid"
            Senv_qid="ori_qid"

        Denv_req="%s_req"%kCmp2
        Denv_resp="%s_resp"%kCmp2
        Denv_eid="%s_eid"%kCmp2
        Denv_qid="%s_qid"%kCmp2
        
        qid_dic[qid]["cmp1_eid"]=qid_dic[qid][Senv_eid]
        qid_dic[qid]["cmp2_eid"]=qid_dic[qid][Denv_eid]
        qid_dic[qid]["cmp1_qid"]=qid_dic[qid][Senv_qid]
        qid_dic[qid]["cmp2_qid"]=qid_dic[qid][Denv_qid]
        qid_dic[qid]["cmp1_resp"]=qid_dic[qid][Senv_resp]
        qid_dic[qid]["cmp2_resp"]=qid_dic[qid][Denv_resp]
        qid_dic[qid]["difference"]=""
        if qid_dic[qid]["cmp1_resp"]=="" and qid_dic[qid]["cmp2_resp"]=="":
            qid_dic[qid]["difference"]="all no response"
        elif qid_dic[qid]["cmp1_resp"]=="":
            qid_dic[qid]["difference"]="%s: no response"%kCmp1
        elif qid_dic[qid]["cmp2_resp"]=="":
            qid_dic[qid]["difference"]="%s: no response"%kCmp2

        if qid_dic[qid]["difference"]=="":
            print "beforeCmpInfoByQid"
            if qid_dic[qid]["cmp1_eid"]==qid_dic[qid]["cmp2_eid"]:
                if qid_dic[qid]["cmp1_eid"]==0:
                    qid_dic[qid]["cmp1_resp"]=json.loads(qid_dic[qid]["cmp1_resp"])
                    qid_dic[qid]["cmp2_resp"]=json.loads(qid_dic[qid]["cmp2_resp"])
                    result=self.__CmpInfo(qid_dic[qid]["ori_req"],qid_dic[qid]["cmp1_resp"],qid_dic[qid]["cmp2_resp"])
                else:
                    result="their error_id = %s"%qid_dic[qid]["cmp1_eid"]
            else:
                result="error_id is diff"

            print "result=%s"%result
            qid_dic[qid]["difference"]=result
        print "CmpInfoByQid ok"

        conn=DBHandle(conf.Storehost,conf.Storeuser,conf.Storepasswd,conf.Storedb)

        if self.kCmp2 not in ("test","test1","offline"):
            Denv_qid="port_qid"
            Denv_resp="port_resp"
        if self.fresh == 1:
            sqlstr="replace into cmp_cases (date,\
                    req_type,\
                    ori_req,\
                    ori_qid,\
                    %s,\
                    %s,\
                    %s,\
                    %s) "%(Senv_qid,Denv_qid,Senv_resp,Denv_resp)
        else:
            sqlstr="replace into cmp_cases (date,\
                    req_type,\
                    ori_req,\
                    %s,\
                    %s,\
                    %s,\
                    %s) "%(Senv_qid,Denv_qid,Senv_resp,Denv_resp)
        if self.fresh ==1:
            sqlstr+="values(%s,%s,%s,%s,%s,%s,%s,%s)"
        else:
            sqlstr+="values(%s,%s,%s,%s,%s,%s,%s)"
        print "sql: ",sqlstr
        args=[]
        if self.fresh==1:
            T=(self.kDate,\
                    self.kType,\
                    qid_dic[qid]["ori_req"],\
                    qid_dic[qid]["ori_qid"],\
                    qid_dic[qid]["cmp1_qid"],\
                    qid_dic[qid]["cmp2_qid"],\
                    json.dumps(qid_dic[qid]["cmp1_resp"]),\
                    json.dumps(qid_dic[qid]["cmp2_resp"]))
        else:
            T=(self.kDate,\
                    self.kType,\
                    qid_dic[qid]["ori_req"],\
                    qid_dic[qid]["cmp1_qid"],\
                    qid_dic[qid]["cmp2_qid"],\
                    json.dumps(qid_dic[qid]["cmp1_resp"]),\
                    json.dumps(qid_dic[qid]["cmp2_resp"]))
        args.append(T)
        conn.do(sqlstr,T)
        print "qid Store ok"
        self.__mymail(qid_dic)

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
        if kQid:
            Senv=self.kCmp1
            Senv_req="%s_req"%Senv
            Senv_resp="%s_resp"%Senv
            Senv_eid="%s_eid"%Senv
            if self.kSource=='bug':
                conn=DBHandle(conf.Storehost,conf.Storeuser,conf.Storepasswd,conf.Storedb)
                sqlstr="select req,resp from error_qid where qid=%s and type='%s' and env='%s';"%(kQid,kType,self.kCmp1)
#                print sqlstr
                reqs=conn.do(sqlstr)
                if len(reqs)==0:
                    print "mysql have no data"
                    exit(1)
                qid_dic[kQid]=dict()
                qid_dic[kQid]["ori_qid"]=kQid
                qid_dic[kQid]["ori_req"]=reqs[0]["req"]
                qid_dic[kQid]["ori_resp"]=reqs[0]["resp"]
                tmp=json.loads(qid_dic[kQid]["ori_resp"])
                qid_dic[kQid]['ori_eid']=tmp['error']['error_id']

            else:
                dbb="logQuery_test"
                conn=DBHandle(conf.Loadhost,conf.Loaduser,conf.Loadpasswd,dbb)
                sqlstr="select req_params from nginx_api_log_%s where qid=%s and log_type='13' and req_type='%s';"%(kDate,kQid,kType)
#                print sqlstr
                reqs=conn.do(sqlstr)
                if len(reqs)==0:
                    print "mysql have no data"
                    exit(1)
                qid_dic[kQid]=dict()
                qid_dic[kQid]["ori_qid"]=kQid
                qid_dic[kQid]["ori_req"]=reqs[0]["req_params"]
                sqlstr="select id from nginx_api_log_%s where qid=%s and log_type='14' and req_type='%s';"%(kDate,kQid,kType)
#                print sqlstr
                ids=conn.do(sqlstr)
                sqlstr="select response from nginx_api_resp_%s where id=%s;"%(kDate,ids[0]["id"])
#                print sqlstr
                resp=conn.do(sqlstr)
                qid_dic[kQid]["ori_resp"]=resp[0]["response"]
#                print "resp=%s"%resp[0]["response"]
                try:
                    tmp=json.loads(qid_dic[kQid]["ori_resp"])
                except:
                    print "pass this resp"
                    return 

                if "error" not in tmp:
                    qid_dic[kQid][Senv_eid]=""
                    print "%s don't have error"%kQid
                    return 
                if "error_id" in tmp["error"]:
                    eid=tmp["error"]["error_id"]
                elif "errorid"in tmp["error"]:
                    print "appear errorid"
                    eid=tmp["error"]["errorid"]
                else:
                    print "no error_id"
                    eid=""
                qid_dic[kQid]["ori_eid"]=eid

        else:
            if self.kSource=='bug':
                conn=DBHandle(conf.Storehost,conf.Storeuser,conf.Storepasswd,conf.Storedb)
                sqlstr="select qid,req,resp from error_qid where date=%s and type='%s' and env='%s' order by rand() limit %d;"%(kDate,kType,self.kCmp1,kNum)
#                print sqlstr
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
                conn=DBHandle(conf.Loadhost,conf.Loaduser,conf.Loadpasswd,'logQuery_test')
                sqlstr="select qid,req_params from nginx_api_log_%s where req_type='%s' and log_type='13' group by qid having count(*)=1 order by rand() limit %d;"%(kDate,kType,kNum)
#                print sqlstr
                qids=conn.do(sqlstr)
                if len(qids)==0:
                    print "mysql have no data"
                    exit(1)
                sqlstr="select qid,id from nginx_api_log_%s where req_type='%s' and log_type='14' and qid in("%(kDate,kType)
                for i,row in enumerate(qids):
                    qid=row["qid"]
                    qid_dic[qid]=dict()
                    qid_dic[qid]["ori_qid"]=qid
                    qid_dic[qid]["req_type"]=kType
                    print row["qid"]
                    print "yuan_req:%s"%row["req_params"]
                    qid_dic[qid]["ori_req"]=row["req_params"]
                    if i == len(qids) - 1:
                        sqlstr+="%s"%row["qid"]
                    else:
                        sqlstr+="%s,"%row["qid"]
                sqlstr+=");"
#                print sqlstr
                ids=conn.do(sqlstr)
                id_dic=dict()
                sqlstr="select id,response from nginx_api_resp_%s where id in ("%kDate
                for i,row in enumerate(ids):
                    id_dic[row["id"]]=row["qid"]
                    if i == len(ids) - 1:
                        sqlstr+="%s"%row["id"]
                    else:
                        sqlstr+="%s,"%row["id"]
                sqlstr+=");"
#                print sqlstr
                resps=conn.do(sqlstr)
                for i,row in enumerate(resps):
                    qid=id_dic[row["id"]]
                    if row["response"]=="":
                        pop_item=qid_dic.pop(qid)
                        self.delete+=1
                        print "no response_delete=%d"%(self.delete)
                        continue
#                    resp=row["response"]
                    qid_dic[qid]["ori_resp"]=row["response"]
#                    print "qid=%s"%qid
                    try:
                        tmp=json.loads(qid_dic[qid]["ori_resp"])
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

    def __Query(self,qid,env,req):     #拿req 打不同的服务器
        qid_dic=self.qid_dic
        env_qid="%s_qid"%env
        env_resp="%s_resp"%env
        env_req="%s_req"%env
        env_eid="%s_eid"%env
        print  10 * "*"
        print "in Query"
        cur_qid=int(round(time.time()*1000))
        qid_dic[qid][env_qid]=cur_qid
        print "ori_qid=%s"%qid
        print "%s_qid=%s"%(env,cur_qid)
        ziduan="qid="+str(cur_qid)
        originqid=r'qid=(\d+)'
        req=re.sub(originqid,ziduan,req)
    #    print "%s=%s"%(env_req,req)
        print "self.fresh=%d"%self.fresh
        qid_dic[qid][env_req]=req
        if env=="test":
            url="http://10.10.135.140:92/?"+req
        elif env=="test1":
            url="http://10.10.135.140:9292/?"+req
        elif env=="online":
            url="http://10.10.135.140:93/?"+req
        elif env=="offline":
            url="http://10.10.135.140:91/?"+req
        else:
            url="http://"+env+"/?"+req
    #    print "%s_url=%s"%(env,url)

        try:
            r=requests.get(url)
            resps=r.text
            r.raise_for_status()
            print "%s=%s"%(env_resp,resps)
            qid_dic[qid][env_resp]=resps
            tmp=json.loads(resps)
            new_eid=tmp["error"]["error_id"]
            print "%s=%s"%(env_eid,new_eid)
            qid_dic[qid][env_eid]=new_eid
        except (requests.RequestException) as e:
            print "requesterror:"
            print e
            qid_dic[qid][env_resp]=""
            qid_dic[qid][env_eid]=None

        except Exception:
            print "url request timeout~"
            qid_dic[qid][env_resp]=""
            qid_dic[qid][env_eid]=None
        self.qid_dic=qid_dic

    def __Parsereq(self):
        kQid=self.kQid
        qid_dic=self.qid_dic
        
        print "in parser"
        if kQid:
            Senv=self.kCmp1
            Denv=self.kCmp2
            Senv_req="%s_req"%Senv
            Senv_resp="%s_resp"%Senv
            req=qid_dic[kQid]["ori_req"]
            ori_uid=re.compile(r'&uid=([^&]+)')
            ziduan="&uid=DataPlan"
            req=re.sub(ori_uid,ziduan,req)
            print "query_req=%s"%req
            print "ori_resp=%s"%(qid_dic[kQid]["ori_resp"])
            self.__Query(kQid,Denv,req)
            if self.fresh==1:
                self.__Query(kQid,Senv,req)
        else:
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
                print "ori_resp=%s"%(qid_dic[qid]["ori_resp"])
                self.__Query(qid,self.kCmp2,req)
                if self.fresh==1:
                    self.__Query(qid,self.kCmp1,req)

    def __Store(self):      #将对比环境的各自响应信息存库
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
                sqlstr="replace into cmp_cases (date,\
                        req_type,\
                        ori_req,\
                        ori_qid,\
                        ori_resp,\
                        port_qid,\
                        port_resp)"
            else:
                sqlstr="replace into cmp_cases (date,\
                        req_type,\
                        ori_req,\
                        ori_qid,\
                        ori_resp,\
                        %s,\
                        %s)"%(Denv_qid,Denv_resp)
        else:
            if self.kCmp2 not in ("test","test1","offline"):
                sqlstr="replace into cmp_cases (date,\
                        req_type,\
                        ori_req,\
                        ori_qid,\
                        %s,\
                        %s,\
                        port_qid,\
                        port_resp)"%(Senv_qid,Senv_resp)
            else:
                sqlstr="replace into cmp_cases (date,\
                        req_type,\
                        ori_req,\
                        ori_qid,\
                        %s,\
                        %s,\
                        %s,\
                        %s)"%(Senv_qid,Senv_resp,Denv_qid,Denv_resp)
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
                        json.dumps(item["cmp1_resp"]),\
                        item[Denv_qid],\
                        json.dumps(item["cmp2_resp"]))
                args.append(T)
        else:
            for qid,item in qid_dic.items():
                T=(kDate,\
                        kType,\
                        item["ori_req"],\
                        item["ori_qid"],\
                        item[Senv_qid],\
                        json.dumps(item["cmp1_resp"]),\
                        item[Denv_qid],\
                        json.dumps(item["cmp2_resp"]))
                args.append(T)

        conn.do(sqlstr,args)
        print "after store"


    def __Fetch(self):    #拿两个环境的response
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

    def __GetResp(self):   #预处理一下error_id 和response,并存结果difference(ByPatch)
        qid_dic=self.qid_dic
        kCmp1=self.kCmp1
        kCmp2=self.kCmp2
        
        for qid,item in qid_dic.items():
            if qid_dic[qid]["cmp1_resp"]=="" and qid_dic[qid]["cmp2_resp"]=="":
                pop_item=qid_dic.pop(qid)
                result="all no resp"
                print "pop: no resp"
                qid_dic[qid]["difference"]=result
                continue;
            elif qid_dic[qid]["cmp1_resp"]=="":
                result="%s: no resp"%kCmp1
                qid_dic[qid]["difference"]=result
                continue;
            elif qid_dic[qid]["cmp2_resp"]=="":
                result="%s: no resp"%kCmp2
                qid_dic[qid]["difference"]=result
                continue;
            if qid_dic[qid]["cmp1_eid"]=="" and qid_dic[qid]["cmp2_eid"]=="":
                print "pop: no eid"
                pop_item=qid_dic.pop(qid)
                continue;
            elif qid_dic[qid]["cmp1_eid"]=="":
                result="%s: no eid"%kCmp1
                qid_dic[qid]["difference"]=result
                continue;
            elif qid_dic[qid]["cmp2_eid"]=="":
                result="%s: no eid"%kCmp2
                qid_dic[qid]["difference"]=result
                continue;
            print "cmp1_qid=%s"%qid_dic[qid]["cmp1_qid"]
            print "cmp2_qid=%s"%qid_dic[qid]["cmp2_qid"]
            print "ori_req=%s"%(qid_dic[qid]["ori_req"])
            print "%s_resp=%s"%(kCmp1,qid_dic[qid]["cmp1_resp"])
            print "%s_resp=%s"%(kCmp2,qid_dic[qid]["cmp2_resp"])
            if qid_dic[qid]["cmp1_eid"]==qid_dic[qid]["cmp2_eid"]:
                if qid_dic[qid]["cmp1_eid"]==0:
                    qid_dic[qid]["cmp1_resp"]=json.loads(qid_dic[qid]["cmp1_resp"],encoding='gbk')
                    qid_dic[qid]["cmp2_resp"]=json.loads(qid_dic[qid]["cmp2_resp"],encoding='gbk')
                    result=self.__CmpInfo(qid_dic[qid]["ori_req"],qid_dic[qid]["cmp1_resp"],qid_dic[qid]["cmp2_resp"])
                else:
                    result="their error_id =%s"%qid_dic[qid]["cmp1_eid"]
            else:
                result="error_id is diff"

            print "result=%s"%result
            if result!="all is the same":
                qid_dic[qid]["difference"]=result
        self.qid_dic=qid_dic
        print "CmpInfoByPatch ok"

    def __CmpProduct(self,cmp1resp,cmp2resp):
        kCmp1=self.kCmp1
        kCmp2=self.kCmp2
        result="all is the same"
        if "product" not in cmp1resp["data"] and "product" not in cmp2resp["data"]:
            return result
        elif "product" not in cmp1resp["data"]:
            result="%s: no product"%kCmp1
            return result
        elif "product" not in cmp2resp["data"]:
            result="%s: no product"%kCmp2
            return result
        cmp1_product=cmp1resp["data"]["product"]
        cmp2_product=cmp2resp["data"]["product"]
        if len(cmp1_product)!=len(cmp2_product):
            if len(cmp1_product)>len(cmp2_product):
                result="%s don't have "%kCmp1
                for key in cmp1_product:
                    if key not in cmp2_product:
                        result+="%s "%key
            else:
                result="%s don't have "%kCmp2
                for key in cmp2_product:
                    if key not in cmp1_product:
                        result+=" %s"%key
            return result

        print "before the key of product cmp"
        for key in cmp1_product:
            if not cmp1_product[key] and not cmp2_product[key]:
                continue;
            elif not cmp1_product[key]:
                result="%s: product[%s] is None"%(kCmp1,key)
                return result
            elif not cmp2_product[key] or key not in cmp2_product:
                result="%s: product[%s] is None"%(kCmp2,key)
                return result
            elif cmp1_product[key] != cmp2_product[key]:
                if len(cmp1_product[key])!=len(cmp2_product[key]):
                    result="length_product[%s]"%key
                    return result
                else:
                    for keyi in cmp1_product[key]:
                        if keyi not in  cmp2_product[key] :
                            result="product[%s][%s] not in %s"%(key,keyi,kCmp2)
                            return result
            else:
                continue
        return "all is the same"

    def __CmpViewDate(self,env,days,cidx):
        print "in CmpViewDate"
        didx=0
        temp=0
        day0Date = datetime.datetime.now()
        while didx <len(days):
#            print "day=%s"%days[didx]
            if "pois" not in days[didx] or not days[didx]["pois"] or "date" not in days[didx] or not days[didx]["date"]:
                print "pois is None or no date"
                didx+=1
                continue
            days_date=days[didx]["date"]
            print "str_day=%s"%days_date
            try:
                if didx==0:
                    day0Date = datetime.datetime(int(days_date[0:4]),int(days_date[4:6]),int(days_date[6:8]))
                didxDate = datetime.datetime(int(days_date[0:4]),int(days_date[4:6]),int(days_date[6:8]))
                if (didxDate-day0Date).days != didx:
                    if cidx==-1:
                        result="%s: date_day[%d] is wrong"%(env,didx)
                    else:
                        result="%s: date_list[%d]day[%d] is wrong"%(env,cidx,didx)
                    return result
                didx+=1
            except:
                return "%s: date error: list[%d]day[%d]"%(env,cidx,didx)
        return "all is the same"

    def __CalRanking(self,days):
        id_ranking=self.id_ranking
        didx=0
        idNum_arvRank=list()
        while didx<len(days):
            totalRanking=0
            avgRanking=0
            if days[didx]["pois"]:
                pois=days[didx]["pois"]
                vidx=0
                z=0
                while vidx<len(pois):
                    id=pois[vidx]["id"]
                    if id_ranking.has_key(id):
                        days[didx]["pois"][vidx]["ranking"]=id_ranking[id]
                        totalRanking+=id_ranking[id]
                        z+=1
                    else:
                        print "id:",id,"has no store ranking"
                    vidx+=1
                avgRanking=(float)(totalRanking)/z if z != 0 else 0.00
                toStr=str(z)+"_"+str('%.2f'%avgRanking)
                idNum_arvRank.append(toStr)
            didx+=1
        print "idNum_arvRank:",idNum_arvRank
        return idNum_arvRank 

    def __CalWholeRanking(self,lists):
        totalNum=0
        totalAvg=0.00
        i=0
        while i<len(lists):
            try:
                days=lists[i]["view"]["summary"]["days"]
            except:
                print "list %d no view_summary_days"%i
                i+=1
                continue
            idNum_arvRank=self.__CalRanking(days)
            for j in idNum_arvRank:
                num,avg=j.split('_')
                totalNum+=int(num)
                totalAvg+=float(avg)*int(num)
            i+=1
        Avg="%.2f"%(totalAvg/totalNum) if totalNum !=0 else 0.00
        print "totalNum:%d, Avg:"%totalNum,Avg
        return '_'.join([str(totalNum),str(Avg)])

    def __CmpView(self,ori_req,cmp1_view,cmp2_view,cidx):
        print "in CmpView"
        try:
            cmp1_days=cmp1_view["summary"]["days"]
            cmp2_days=cmp2_view["summary"]["days"]
        except:
            result="no sumary_days"
            print "no summary_days"
            return result
        result="all is the same"
        if (not cmp1_days or cmp1_days=='None') and (not cmp2_days or cmp2_days=='None'):
            return result
        elif not cmp1_days or cmp1_days=='None':
            if cidx!=-1:
                result="%s: days_list[%d] is None"%(self.kCmp1,cidx)
            else:
                result="%s: days is None"%(self.kCmp1)
        elif not cmp2_days or cmp2_days=='None':
            if cidx!=-1:
                result="%s: days_list[%d] is None"%(self.kCmp2,cidx)
            else:
                result="%s: days is None"%(self.kCmp2)
        if result != "all is the same":
            return result
        #比较checkin和checkout
        pat_query=re.compile(r'&query=([^&]+)')
        match_query=pat_query.search(ori_req)
        if match_query:
            query=match_query.group(1)
            query=urllib2.unquote(query.decode('utf-8','replace').encode('gbk','replace'))
            query=json.loads(query)
            if self.kType=='s127':
                List=query["list"]
                checkin=List[cidx]["checkin"]
                checkout=List[cidx]["checkout"]

            else:
                checkin=query["city"]["checkin"]
                checkout=query["city"]["checkout"]

            if (checkin and checkin != "null") and (checkout and checkout != "null"):
                print "checkin=",checkin
                print "checkout=",checkout
                kin=datetime.datetime.strptime(checkin,'%Y%m%d')
                kout=datetime.datetime.strptime(checkout,'%Y%m%d')
                delta=kout-kin
                during=delta.days+1
            else:
                during=1
            print "during: ",during
            print "cmp1_days len: ",len(cmp1_days)
            print "cmp2_days len: ",len(cmp2_days)
            if during != len(cmp1_days):
                if cidx !=-1:
                    result="%s: list[%d] checkin_checkout error"%(self.kCmp1,cidx)
                else:
                    result="%s: checkin_checkout error"%self.kCmp1
                return result
            if during != len(cmp2_days):
                if cidx != -1:
                    result="%s: list[%d] checkin_checkout error"%(self.kCmp2,cidx)
                else:
                    result="%s: checkin_checkout error"%self.kCmp2
                return result

#        print "cidx=%d"%cidx
        result=self.__CmpViewDate(self.kCmp1,cmp1_days,cidx)
        print "cmp1ViewDate ok"
        if result=="all is the same":
            result=self.__CmpViewDate(self.kCmp2,cmp2_days,cidx)
        else:
            tmp=self.__CmpViewDate(self.kCmp2,cmp2_days,cidx)
            if tmp!="all is the same":
                result=result+', '+tmp
        print "CmpViewDate ok"

        if result!="all is the same":
            return result
        if len(cmp1_days)!=len(cmp2_days):
            if cidx==-1:
                result="length_days is diff"
            else:
                result="length_list[%d]days is diff"%cidx
            return result
        else:
            rankResult="the same avg_ranking"
            if self.kType == 's127':
                cmp1_idNum_avgRank=self.__CalRanking(cmp1_days)
                cmp2_idNum_avgRank=self.__CalRanking(cmp2_days)
                print "cmp1_list[%d]IdNum_avgRank:"%cidx,cmp1_idNum_avgRank
                print "cmp2_list[%d]IdNum_avgRank:"%cidx,cmp2_idNum_avgRank
                if cmp1_idNum_avgRank != cmp2_idNum_avgRank:
                    rankResult=", cmp1_list[%d]IdNum_avgRank: "%cidx+str(cmp1_idNum_avgRank)+", cmp2_list[%d]IdNum_avgRank: "%cidx+str(cmp2_idNum_avgRank)
                    print "rankResult:",rankResult

            didx=0
            z=0
            while didx<len(cmp1_days):
                if ("pois" not in cmp1_days[didx] or not cmp1_days[didx]["pois"]) and ("pois" not in cmp2_days[didx] or not cmp2_days[didx]["pois"]):
                    didx+=1
                    continue
                elif "pois" not in cmp1_days[didx] or not cmp1_days[didx]["pois"]:
                    if cidx==-1:
                        result="%s: pois of day[%d] is None"%(self.kCmp1,didx)
                    else:
                        result="%s: pois of list[%d]day[%d] is None"%(self.Kcmp1,cidx,didx)
                elif "pois" not in cmp2_days[didx] or not cmp2_days[didx]["pois"]:
                    if cidx==-1:
                        result="%s: pois of day[%d] is None"%(self.kCmp2,didx)
                    else:
                        result="%s: pois of list[%d]day[%d] is None"%(self.Kcmp2,cidx,didx)
                    if result != "all is the same":
                        if rankResult != "the same avg_ranking":
                            result=result+rankResult
                        return result

                cmp1_pois=cmp1_days[didx]["pois"]
                cmp2_pois=cmp2_days[didx]["pois"]
                if len(cmp1_pois)!=len(cmp2_pois):
                    if cidx==-1:
                        result="length_pois of day[%d]"%(didx)
                    else:
                        result="length_pois of list[%d]day[%d]"%(cidx,didx)
                    if rankResult != "the same avg_ranking":
                        result=result+rankResult
                    return result
                else:
                    vidx=0
                    while vidx<len(cmp1_pois):
                        cmp1_id=cmp1_pois[vidx]["id"]
                        cmp2_id=cmp2_pois[vidx]["id"]
                        cmp1_pdur=cmp1_pois[vidx]["pdur"]
                        cmp2_pdur=cmp2_pois[vidx]["pdur"]
                        if cmp1_id != cmp2_id:
                            if cidx!=-1:
                                result="id_list[%d]day[%d]pois[%d]"%(cidx,didx,vidx)
                            else:
                                result="id_day[%d]pois[%d]"%(didx,vidx)
                            if rankResult != "the same avg_ranking":
                                result=result+rankResult
                            return result
                        if cmp1_pdur != cmp2_pdur:
                            z+=1
                            if(z>5):
                                vidx+=1
                                continue;
                            if z==1:
                                if cidx!=-1:
                                    result="pdur_list[%d]day[%d]pois[%d]"%(cidx,didx,vidx)
                                else:
                                    result="pdur_day[%d]pois[%d]"%(didx,vidx)
                            else:
                                if cidx!=-1:
                                    result+=", pdur_list[%d]day[%d]pois[%d]"%(cidx,didx,vidx)
                                else:
                                    result+=", pdur_day[%d]pois[%d]"%(didx,vidx)
                        vidx+=1
                didx+=1
            if rankResult != "the same avg_ranking":
                result=result+rankResult
            return result
            print "CmpView ok"

    def __CmpInfo(self,ori_req,cmp1resp,cmp2resp):
        kCmp1=self.kCmp1
        kCmp2=self.kCmp2
        kType=self.kType
        #s128,s130 不一定有city //data->city->view->summary->days->pois
        #s127                   //data->list->view->summary->days->pois
        #s125                   //data->view->summary->days->pois
        result="all is the same"
        if "data" not in cmp1resp and "data" not in cmp2resp:
            result="error: all no data"
            return result
        elif "data" not in cmp1resp:
            result="%s: no data"%kCmp1
            return result
        elif "data" not in cmp2resp:
            result="%s: no data"%kCmp2
            return result

        if not cmp1resp["data"] and not cmp2resp["data"]:
            result="error: all no data"
            return result
        elif not cmp1resp["data"]:
            result="%s: data is None"%kCmp1
            return result
        elif not cmp2resp["data"]:
            result="%s: data is None"%kCmp2
            return result

        if kType=="s127":
            print "in s127 view_cmp"
            if "list" not in cmp1resp["data"] and "list" not in cmp2resp["data"]:
                return result
            elif "list" not in cmp1resp["data"]:
                result="%s: no list"%kCmp1
                return result
            elif "list" not in cmp2resp["data"]:
                result="%s: no list"%kCmp2
                return result
            if len(cmp1resp["data"]["list"])!=len(cmp2resp["data"]["list"]):
                result="length_list"
                return result
            AvgRankResult="the same average ranking"
            cmp1_whole_Rank=self.__CalWholeRanking(cmp1resp["data"]["list"]);
            cmp2_whole_Rank=self.__CalWholeRanking(cmp2resp["data"]["list"]);
            print "cmp1_whole_Rank:",cmp1_whole_Rank,"cmp2_whole_Rank:",cmp2_whole_Rank
            if cmp1_whole_Rank != cmp2_whole_Rank:
                AvgRankResult=", cmp1_totalIdNum_avgRank: "+cmp1_whole_Rank+", cmp2_totalIdNum_avgRank: "+cmp2_whole_Rank
                print "AvgRankResult:",AvgRankResult
                num1,avg1=cmp1_whole_Rank.split('_')
                num2,avg2=cmp2_whole_Rank.split('_')
                self.AllQidIdNum1+=int(num1)
                self.AllQidIdNum2+=int(num2)
                self.AllQidAvg1+=float(avg1)*int(num1)
                self.AllQidAvg2+=float(avg2)*int(num2)
            i=0
            while i<len(cmp1resp["data"]["list"]):
                if not cmp1resp["data"]["list"][i]["view"] and not cmp2resp["data"]["list"][i]["view"]:
                    i+=1
                    continue;
                elif not cmp1resp["data"]["list"][i]["view"] and cmp2resp["data"]["list"][i]["view"]:
                    self.view_num+=1
                    self.error_num+=1
                    print "cmp1_view is null"
                    result="%s: view_list[%d] is None"%(kCmp1,i)
                    return result
                elif cmp1resp["data"]["list"][i]["view"] and not cmp1resp["data"]["list"][i]["view"]:
                    self.view_num+=1
                    self.error_num+=1
                    result="%s: view_list[%d] is None"%(kCmp2,i)
                    return result
                else:
                    cmp1_view=cmp1resp["data"]["list"][i]["view"]
                    cmp2_view=cmp2resp["data"]["list"][i]["view"]
                    result=self.__CmpView(ori_req,cmp1_view,cmp2_view,i)
                    if result != "all is the same":
                        self.view_num+=1
                        self.error_num+=1
                        if AvgRankResult != "the same average ranking":
                            result=result+AvgRankResult
                        return result
                i+=1
            if result != "all is the same":
                return result

        elif kType=="s125":
            print "in s125 view_cmp"
            if "view" not in cmp1resp["data"] and "view" not in cmp2resp["data"]:
                return "all is the same"
            elif "view" not in cmp1resp["data"]:
                self.view_num+=1
                self.error_num+=1
                result="%s: no view"%kCmp1
                return result
            elif "view"  not in cmp2resp["data"]:
                self.view_num+=1
                self.error_num+=1
                result="%s: no view"%kCmp2
                return result
            cmp1_view=cmp1resp["data"]["view"]
            cmp2_view=cmp2resp["data"]["view"]
            result=self.__CmpView(ori_req,cmp1_view,cmp2_view,-1)
            if result != "all is the same":
                self.view_num+=1
                self.error_num+=1
                return result
        else:
            print "before s128 or s130 view_cmp"
            if "city" not in cmp1resp["data"] and "city" not in cmp2resp["data"]:
                return result
            elif "city" not in cmp1resp["data"]:
                result="%s: no city"%kCmp1
                return result
            elif "city" not in cmp2resp["data"]:
                result="%s: no city"%kCmp2
                return result

            if ("view" not in cmp1resp["data"]["city"] or not cmp1resp["data"]["city"]["view"]) and ("view" not in cmp2resp["data"]["city"] or not cmp2resp["data"]["city"]["view"] ):
                print "all no view"
                return "all is the same"
            elif "view" not in cmp1resp["data"]["city"] or not cmp1resp["data"]["city"]["view"]:
                self.view_num+=1
                self.error_num+=1
                result="%s: no view"%kCmp1
                return result
            elif "view" not in cmp2resp["data"]["city"] or not cmp2resp["data"]["city"]["view"]:
                self.view_num+=1
                self.error_num+=1
                result="%s: no view"%kCmp2
                return result
            cmp1_view=cmp1resp["data"]["city"]["view"]
            cmp2_view=cmp2resp["data"]["city"]["view"]
            result=self.__CmpView(ori_req,cmp1_view,cmp2_view,-1)
            if result != "all is the same":
                self.view_num+=1
                self.error_num+=1
                return result
        print "before product cmp"
        result=self.__CmpProduct(cmp1resp,cmp2resp)
        if result != "all is the same":
            self.product_num+=1
            self.error_num+=1
        return result


