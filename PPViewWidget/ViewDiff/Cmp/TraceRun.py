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
import TraceQidTool

def main():
    fresh=0
    parser = optparse.OptionParser()
    parser.add_option('-d', '--date', help = 'run date like 20150101')
    parser.add_option('-s', '--stime', help = 'run like 00:00:00')
    parser.add_option('-e', '--etime', help = 'run like 23:59:59')
    parser.add_option('-u', '--uid', help = 'run type like DataPlan')
    parser.add_option('-t', '--type', help = 'run type like s125')
    parser.add_option('-o', '--env', type="string",dest="env",help='the cmp_env run like test/offline or offline')
    parser.add_option('-m', '--mail', type="string",dest="mail",help='mail to like caoxiaolan@mioji.com ')
    opt, args = parser.parse_args()
    print "len of argv=%d"%len(sys.argv)
    print opt.date
    print opt.uid
    print opt.type
    print opt.env
    print opt.mail
    Date=""
    Uid=""
    Type=""
    Env=""
    Mail=""
        
    try:    
        run_day = datetime.datetime.fromtimestamp(time.mktime(time.strptime(opt.date, '%Y%m%d')))
        today = datetime.datetime.today()
        print run_day
        if (today-run_day).days > 4:
            print "the date is wrong, please input date less five days than ",datetime.date.today()
            return
        date_str = run_day.strftime('%Y%m%d')
        Date = date_str;
    except:
        print "Error Format date"
        return
    stime=Date+" "+opt.stime
    etime=Date+" "+opt.etime
    dayBegin=Date+" "+"00:00:00"
    dayEnd=Date+" "+"23:59:59"
    stimeStamp=time.mktime(time.strptime(stime,"%Y%m%d %H:%M:%S"))
    etimeStamp=time.mktime(time.strptime(etime,"%Y%m%d %H:%M:%S"))
    beginStamp=time.mktime(time.strptime(dayBegin,"%Y%m%d %H:%M:%S"))
    endStamp=time.mktime(time.strptime(dayEnd,"%Y%m%d %H:%M:%S"))
    if stimeStamp < beginStamp or etimeStamp > endStamp :
        print "Error Time Range"
        return
    else:
        Sqid=int(stimeStamp)*1000
        Eqid=int(etimeStamp)*1000

    try:
        Uid=opt.uid
    except:
        print "no uid"
        return 
    if opt.type:
        if opt.type in('s125','s127','s128','s130','p101','p104','p105'):
            Type=opt.type
        else:
            print "Error Formate type"
            return 

    if opt.env in('test','online','offline'):
        Env=opt.env
    else:
        print "Error environment"
        return

    m=re.compile(r'(.+)@mioji.com')
    match_mail=m.search(opt.mail)
    if match_mail:
        Mail=opt.mail
    else:
        print "MailBox Error"
        return 
    Trace=TraceQidTool.TraceQid(Date,Sqid,Eqid,Uid,Type,Env,Mail)
    Trace.LoadData()
    Trace.Store()
    Trace.Mymail()

if __name__ == "__main__":
    main()


