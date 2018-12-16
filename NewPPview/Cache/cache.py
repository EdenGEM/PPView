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
import io
import urllib2
sys.path.append('../Common')
from DBHandle import DBHandle
reload(sys);
exec("sys.setdefaultencoding('utf-8')");

def Report():
    qid_dic=dict()
    dbb="logQuery_%s"%kEnv
    conn=DBHandle('10.10.107.246','reader','miaoji1109',dbb)
    sqlstr="select req_params from nginx_api_log_%s where qid=%s and log_type='13' and req_type='%s';"%(kDate,kQid,kType)
    req=conn.do(sqlstr)
    if len(req)==0:
        print "no req"
        return
    qid_dic[kQid]=dict()
    qid_dic[kQid]["req"]=req[0]["req_params"]
    sqlstr="select id from nginx_api_log_%s where qid=%s and log_type='14' and req_type='%s';"%(kDate,kQid,kType)
    ids=conn.do(sqlstr)
    sqlstr="select response from nginx_api_resp_%s where id=%s;"%(kDate,ids[0]['id'])
    resp=conn.do(sqlstr)
    if len(resp)==0:
        print "no resp"
        return
    qid_dic[kQid]["resp"]=resp[0]['response']
    conn=DBHandle('10.10.169.10','root','miaoji1109','viewdiff')
    sqlstr="replace into error_qid (env,date,type,qid,req,resp) values(%s,%s,%s,%s,%s,%s);"
    args=list()
    T=(kEnv,kDate,kType,kQid,qid_dic[kQid]["req"],qid_dic[kQid]["resp"])
    args.append(T)
    conn.do(sqlstr,args)


kQid=''
kEnv=''
kDate=''
kType=''
def main():
    global kDate
    global kQid
    global kEnv
    global kType
    parser = optparse.OptionParser()
    parser.add_option('-q', '--qid' )
    parser.add_option('-t', '--type' )
    parser.add_option('-e', '--env', type="string",default="test",help = 'run env like test')
    opt, args = parser.parse_args()
    tt=int(opt.qid)
    kDate=time.strftime("%Y%m%d",time.localtime(tt/1000))
#    print kDate
    if opt.env=="online" or opt.env=="offline" or opt.env=="test" or opt.env=="test1":
        kEnv=opt.env
    else:
        print "Error Format env"
        return 
    if len(opt.qid)!=13:
        print "Error Format qid"
        return
    else:
        kQid=opt.qid
    if opt.type not in ('s125','s127','s128','s129','s130','p101','p104','p105','b116'):
        print "Error Format type"
        return
    else:
        kType=opt.type
    Report();

if __name__=='__main__':
    main()

