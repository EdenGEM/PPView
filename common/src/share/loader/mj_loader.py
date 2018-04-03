#coding=utf-8

import requests
import time
import threading
import commands
from threading import Timer 
import re
import sys
import urllib
import json


########READ ME#######
#务必在当前目录下生成一个case.list文件
#把请求的query字段放到该文件中即可
#日志输出在test.result中
####### END ##########

###### 配置文件区 ######
g_type = 'tsv003'
g_method = 'get'
#请求QPS
g_qps = 1
#压力测试发请求的总时长 单位秒
g_dur = 1
#日志打印级别
g_log_debug_lvl = 0    #0:精简日志 1:输出详细日志（带返回结果）  
#是否检查error_id (妙计专用测试属性 返回结果必须是json)
g_check_error_id = 0    #0:不检查error_id 1:检查error_id

#######请求PHP配置区 后台测试不需要关注这些##########
g_token = '4dc0db5b9cf87209dc0d978efe83b0e1'
#测试
# g_csuid = 'j6m1anpt57de379391c4f688id115lk8'
# g_uid = '2vrqcopt57de377002617779id10blq3'
# g_tid = 'yikhstpt57de37714b00d165id12zrch'
# g_url = 'http://testapi.mioji.com/city'
#线上
# g_csuid = 'y6xnpzpt57e3d9d0b10f2129id119rqt'
# g_uid = 'ylrq1dpt57e498107c7d6447id10f47e'
# g_tid = '6g7ujnpt5834124bcfd14p03id1252mu'
# g_url = 'http://bapi.mioji.com/city'
#开发
g_csuid = 'y6xnpzpt57e3d9d0b10f2129id119rqt'
g_uid = 'ylrq1dpt57e498107c7d6447id10f47e'
g_tid = '6g7ujnpt5834124bcfd14p03id1252mu'
# g_url = 'http://10.19.20.200:48067'
# g_url = 'http://10.10.135.140:91'
g_url = 'http://10.10.122.15:48066'
################ END ##########################




###### 全局变量区 ######
#请求当前数量
g_cnt = 0  
#未处理完的线程
g_cnt_thread = 0  
#返回异常数量
g_cnt_failed = 0  
#返回总数目
g_cnt_ret = 0 
#当前返回结果的最大长度
g_max_response_len = 0
#累积的总请求时间 单位毫秒
g_total_time = 0.0
#请求间隔
g_interval = float(1000.0/g_qps)/1000.0
#请求总数
g_total_request = int(g_qps*g_dur) if g_qps*g_dur > 1.0 else 1
#请求query字段（文件读取）
g_case_list = []
#qid 防止重复
g_qid = int(time.time()*1000)
#是否单一case测试
g_same_case = False



#输出文件
# backfile = 'log_%s' % time.strftime( '%Y%m%d_%X', time.localtime( time.time() ) )
# commands.getstatusoutput('mv log %s' % (backfile))
g_file_log = open('test.result', 'w')



def getQID():
        global g_qid
        g_qid += 1
        return g_qid

def readCases():
        global g_case_list
        global g_same_case
        file_case = open('case.list','r')
        rep_tid_str = '"tid":"%s"' % (g_tid)
        for line in file_case:
                if line[0] == '#':
                        continue
                if len(line) < 5:
                        continue
                line = re.sub('"tid":".*?"',rep_tid_str,line)
                g_case_list.append(line)
        if len(g_case_list) <= 1:
                g_same_case = True;
        file_case.close()

class base_thread(threading.Thread):
        def __init__(self, func, idx):
                threading.Thread.__init__(self)
                self.func = func
                self.__idx = idx
        def run(self):
                self.func(self.__idx)


def checkErrorID(content):
        global g_check_error_id
        if g_check_error_id == 0:
                return True
        else:
                j = json.loads(content)
                if j.has_key("error") and j["error"].has_key("error_id") and j["error"]["error_id"] == 0:
                        return True
                else:
                        return False

def checkLength(respLen):
        global g_same_case
        if g_same_case:
                rate = float(abs(g_max_response_len - respLen))/g_max_response_len
                if rate > 0.3:
                        return False
        return True

def add2Result(idx, cost_time, respLen, content):
        global g_cnt_failed
        global g_max_response_len
        global g_total_time
        global g_cnt_ret
        

        g_cnt_ret += 1
        if g_max_response_len == 0:
                g_max_response_len = respLen
        
        g_total_time += cost_time
        #判断是否异常
        if not (checkErrorID(content) and checkLength(respLen)):
                g_cnt_failed += 1
        
        #找返回结果长度最大值
        if g_max_response_len < respLen:
                g_max_response_len = respLen


def doRequest(idx):
        global g_cnt_failed
        global g_cnt_thread
        qid = getQID()
        headers = {'token':g_token,'connection':"close"}
        data = {'type':g_type,
                'dev':'0',
                'ver': '',
                'lang':'zh_cn',
                'ccy':'CNY',
                'qid': qid,
                'ptid':'ptid',
                'net': 2,
                'csuid': g_csuid,
                'uid': g_uid,
                'query': g_case_list[idx%len(g_case_list)]
                }
        print("请求<%d>发送 qid:%d\n"%(idx,data['qid']))
        start = int(time.time()*1000)
        if g_method == 'get':
                r = requests.get(g_url,params=data)
        else:        
                r = requests.post(g_url,headers=headers,data=data)
        #计算响应时间
        cost_time = float(int(time.time()*1000) - start)/1000
        #打印请求URL
        #print(r.url)
        # 获取返回结果及长度
        content = r.text
        respLen = len(content)
        print("请求<%d>响应 len:%d cost:%.3f\n"%(idx,respLen,cost_time))
        add2Result(idx, cost_time, respLen, content)
        if g_log_debug_lvl == 0:
                content = "idx:"+str(idx)+" len:"+ str(respLen)+" qid:"+str(qid) + "\n"
        else:
                content = "idx:"+str(idx)+" len:"+ str(respLen)+" qid:"+str(qid)+ ":" + content + "\n"        
        g_file_log.write(content)
        g_cnt_thread -= 1



def test_load():
        global g_cnt
        global g_total_request
        global g_interval
        global g_cnt_thread
        while g_cnt < g_total_request:
                g_cnt += 1
                g_cnt_thread += 1
                idx = g_cnt
                task = base_thread(doRequest,idx)
                task.start()
                time.sleep(g_interval)


def reinit():
        #请求当前数量
        g_cnt = 0  
        #请求失败数量
        g_cnt_failed = 0  
        #请求间隔
        g_interval = float(1000.0/g_qps)/1000.0
        #请求总数
        g_total_request = int(g_qps*g_dur) if g_qps*g_dur > 1.0 else 1


def makeResult():
        global g_cnt_failed
        print("#######结论######")
        conclusion = "总请求 %d 个,正常返回结果 %d 个,结果异常 %d 个,丢失请求 %d 个\n" % (g_total_request,g_cnt_ret-g_cnt_failed,g_cnt_failed,g_total_request-g_cnt_ret)
        conclusion += 'QPS:%.2f 平均响应时间:%.3fs 请求失败率:%.2f%%\n' % (g_qps,g_total_time/g_cnt_ret,(g_total_request-g_cnt_ret+g_cnt_failed)*100/g_total_request)
        print(conclusion)
        g_file_log.write("\n\n#######结论######\n")
        g_file_log.write(conclusion)

def main():
        global g_qps
        global g_dur
        global g_interval
        global g_total_request
        global g_cnt
        global g_cnt_thread
        #读取请求列表
        readCases()
        if True:
                #初始化测试参数
                reinit()
                #测试开始
                print("测试开始\n\tQPS:%.2f\n\t测试时间间隔:%.3fs\n\t测试请求数:%d\n" % (g_qps, g_interval,g_total_request))
                test_load()
                #等待所有结果返回
                while g_cnt_thread > 0:
                        time.sleep(0.2)
                #生成结果
                makeResult()


main()
g_file_log.close()











