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
import conf
import commands
import requests
sys.path.append('../../Common')
from DBHandle import DBHandle

reload(sys);
exec("sys.setdefaultencoding('utf-8')");

class TraceQid:
    Types_dic=dict()

    def __init__(self,kDate="",kSqid="",kEqid="",kUid="",kType="",kEnv="",Mail=""):
        self.kDate=kDate
        self.kSqid=kSqid
        self.kEqid=kEqid
        self.kUid=kUid
        self.kType=kType
        self.kEnv=kEnv
        self.kMail=Mail

    def LoadData(self):
        Types_dic=self.Types_dic
        if self.kType == "":
            Types=('s125','s127','s128','s130','p101','p104','p105')
            for Type in Types:
                print "Type:",Type
                Types_dic[Type]=dict()
                Types_dic[Type]=self.__LoadOneTypeData(Type)
                print "qid_dic size:",len(Types_dic[Type])
        else:
            print "Type:",self.kType
            Types_dic[self.kType]=dict()
            Types_dic[self.kType]=qid_dic
            print "qid_dic size:",len(Types_dic[self.kType])

        self.Types_dic=Types_dic


    def __LoadOneTypeData(self,Type):
        kUid=self.kUid
        kDate=self.kDate
        kEnv=self.kEnv
        kSqid=self.kSqid
        kEqid=self.kEqid
        qid_dic=dict()

        dbb="logQuery_"+kEnv
        conn=DBHandle(conf.Loadhost,conf.Loaduser,conf.Loadpasswd,dbb)
        sqlstr="select qid,req_params from nginx_api_log_%s where uid='%s' and req_type='%s' and log_type='13' and qid > %s and qid <%s and req_params is not null group by qid having count(*)=1;"%(kDate,kUid,Type,kSqid,kEqid)
#            sqlstr="select qid,req_params from nginx_api_log_%s where uid='%s' and log_type='13' and req_type='%s' and req_params is not null;"%(kDate,kUid,kType)
        print "sqlstr:",sqlstr
        qids=conn.do(sqlstr)
        if len(qids)==0:
            print "mysql have no data"
            return dict()
        sqlstr="select qid,id from nginx_api_log_%s where uid='%s' and req_type='%s' and log_type='14' and qid in("%(kDate,kUid,Type)
        for i,row in enumerate(qids):
            qid=row["qid"]
            qid_dic[qid]=dict()
            qid_dic[qid]["req_type"]=Type
            print row["qid"]
#            print "yuan_req:%s"%row["req_params"]
            qid_dic[qid]["req"]=row["req_params"]
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
                print "qid: %s no response"%qid
                continue
            qid_dic[qid]["resp"]=row["response"]

        return qid_dic

    def Store(self):
        Date=self.kDate
        Uid=self.kUid
        Env=self.kEnv

        Types_dic=self.Types_dic
        conn=DBHandle(conf.Storehost,conf.Storeuser,conf.Storepasswd,conf.Storedb)
        sqlstr="replace into trace_qid (date,env,uid,req_type,qid,req,resp) values(%s,%s,%s,%s,%s,%s,%s)"
        args=[]
        for Type,item in Types_dic.items():
            for qid,value in item.items():
                T=(Date,\
                        Env,\
                        Uid,\
                        Type,\
                        qid,\
                        value["req"],\
                        value["resp"])
                args.append(T)
        conn.do(sqlstr,args)
        print "Store ok"


    def Mymail(self):
        Types_dic=self.Types_dic
        qids_dic=dict()
        for Type,values in Types_dic.items():
            for qid,item in values.items():
                if not qids_dic.has_key(qid):
                    qids_dic[qid]=list()
                Type_dic=dict()
                Type_dic[Type]=dict()
                Type_dic[Type]["qid"]=qid
                Type_dic[Type]["req_type"]=Type
                Type_dic[Type]["req"]=item["req"]
                Type_dic[Type]["resp"]=item["resp"]
                qids_dic[qid].append(Type_dic)

        print "qids_dic size: ",len(qids_dic)
        for i in qids_dic:
            print i
        Dict=sorted(qids_dic.items(),key=lambda x:x[0])
        print 'after sort'
        for i in Dict: 
            print i

        env=self.kEnv
        uid=self.kUid
        if len(self.Types_dic) == 0:
            html='''
            <html><body>
            <h1 align="center">No data</h1>
            </body></html>
            '''
            return 0

        html='''
        <html><body>
        <h1 align="center">Trace env_uid:%s_%s</h1>
        <table border='1' align="center">
        <tr><th>type</th><th>query</th><th>req</th><th>resp</th></tr>
        <br>
        '''%(self.kEnv,self.kUid)
        for Type,item in Types_dic.items():
            for qid,value in item.items():
                    html+='''
                    <tr>
                        <td>%s</td>
                        <td>
                        <a href="http://10.10.191.51:8000/cgi-bin/trace.py?env=%s&uid=%s&qid=%s&type=%s&load=query" target="_blank">%s</a>
                        </td>
                        <td>
                        <a href="http://10.10.191.51:8000/cgi-bin/trace.py?env=%s&uid=%s&qid=%s&type=%s&load=req" target="_blank">%s</a>
                        </td>
                        <td>
                        <a href="http://10.10.191.51:8000/cgi-bin/trace.py?env=%s&uid=%s&qid=%s&type=%s&load=resp" target="_blank">%s</a>
                        </td>
                    </tr>
                    '''%(Type,env,uid,str(qid),Type,str(qid),\
                            env,uid,str(qid),Type,str(qid),\
                            env,uid,str(qid),Type,str(qid))
        html+='</table>'
        html+='</body></html>'
        with io.open('./traceQid.html','w',encoding='utf-8') as f:
            f.write((html).decode('utf-8'))
        print "Report ok"
        Dir=commands.getoutput('pwd')
        Report=Dir+"/traceQid.html"
#        print Report
        mailto=self.kMail
        commands.getoutput('mioji-mail -m '+mailto+' -b "CmpInfo" -f '+Report+' -h 123')
        print "mail ok"



