#!/usr/local/bin//python
#coding=utf-8

import io
import os
import re
import sys
import optparse
import time
import datetime
import MySQLdb
import SuperCmpResp

def main():
    fresh=0
    parser = optparse.OptionParser()
    parser.add_option('-d', '--date', help = 'run date like 20150101')
    parser.add_option('-t', '--type', help = 'run type like s125')
    parser.add_option('-n', '--num',type="int",dest="num",default="20", help = 'run case number like 100')
    parser.add_option('-q', '--qid',type="string",dest="qid")
    parser.add_option('-e', '--env', type="string",dest="env",help='the cmp_env run like test/offline or offline')
    parser.add_option('-s', '--source', type="string",dest="source",default='log',help='the source of load req and resp,run like log or bug')
    parser.add_option('-m', '--mail', type="string",dest="mail",help='mail to like caoxiaolan@mioji.com ')
    opt, args = parser.parse_args()
    print "len of argv=%d"%len(sys.argv)
    print opt.date
    print opt.type
    print opt.num
    print opt.qid
    print opt.env
    print opt.source
    print opt.mail
    kQid=""
    kDate=""
#    kType=""
    kNum=1
    if opt.source not in ('log','bug'):
        print "Error Format source"
        return 
    else:
        kSource=opt.source
    if opt.qid:
        print "before qid"
        if len(opt.qid)!=13:
            print "Error Formate qid"
            return 
        else:
            kQid=opt.qid
            print kQid
    else:
        print "before patch"
        if kSource == "log":
            try:    
                run_day = datetime.datetime.fromtimestamp(time.mktime(time.strptime(opt.date, '%Y%m%d')))
                today = datetime.datetime.today()
                print run_day
                if (today-run_day).days > 4:
                    print "the date is wrong, please input date less five days than ",datetime.date.today()
                    return
                date_str = run_day.strftime('%Y%m%d')
                kDate = date_str;
            except:
                print "Error Format date"
                return
        try:
            kNum=opt.num
        except:
            print "Error Formate case number"
            return
    if opt.type in ("s225","s227","s228","s230","p201","p202","p204","p205"):
        kType=opt.type
    else:
        print "Error Formate type"
        return 
    a=opt.env.split('/')
    if len(a)==2:
        kCmp1=a[1]
        kCmp2=a[0]
        fresh=1
    else:
        kCmp1="ori"
        kCmp2=a[0]
    m=re.compile(r'(.+)@mioji.com')
    match_mail=m.search(opt.mail)
    if match_mail:
        Mail=opt.mail
    else:
        print "MailBox Error"
        return 
    print "fresh=%d"%fresh
    if kSource !='bug':
        Boss=SuperCmpResp.RespCmp(kSource,kDate,kType,kNum,kQid,kCmp1,kCmp2,Mail,fresh)
    else:
        Boss=SuperCmpResp.RespCmp(kSource,kDate,kType,kNum,kQid,kCmp1,kCmp2,Mail,0)

    Boss.LoadReqAndResp()
    Boss.CmpInfo()


if __name__ == "__main__":
    main()


