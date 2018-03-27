#!/usr/local/bin//python
#-*- coding: utf-8 -*-
import json
import io
import commands
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

def main():
    routeJ_dic=list()
    #正常routeJ
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v2":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[33,44]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    routeJ.append(string1)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)
    
    #0.时间颠倒
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v2":10},"fixed":{"times":[[33,44]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    routeJ.append(string1)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #0.时间重叠
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v2":10},"fixed":{"times":[[11,44]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[22,33]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    routeJ.append(string1)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #2.时间不足
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v3":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string3="""{"id":"v3","locations":{"in":"30,40","out":"30,40"},"toNext":{"v2":20},"free":{"openClose":[[33,64]],"durs":[30,40,50]},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[40,84]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    string3=json.loads(string3)
    routeJ.append(string1)
    routeJ.append(string3)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #3.交通时间不足
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v2":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[30,44]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    routeJ.append(string1)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #4.非锁定时刻点无法满足开关门
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v3":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string3="""{"id":"v3","locations":{"in":"30,40","out":"30,40"},"toNext":{"v2":20},"free":{"openClose":[[20,30]],"durs":[10,20,30]},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[60,70]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    string3=json.loads(string3)
    routeJ.append(string1)
    routeJ.append(string3)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #5.锁定时刻点无法在指定时刻或之前到达
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v3":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string3="""{"id":"v3","locations":{"in":"30,40","out":"30,40"},"toNext":{"v2":20},"free":{"openClose":[[40,50]],"durs":[10,20,30]},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[60,70]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    string3=json.loads(string3)
    routeJ.append(string1)
    routeJ.append(string3)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #6.新增点离开时间和右边界冲突
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v3":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string3="""{"id":"v3","locations":{"in":"30,40","out":"30,40"},"toNext":{"v2":20},"free":{"openClose":[[40,80]],"durs":[30,40,50]},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[50,60]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    string3=json.loads(string3)
    routeJ.append(string1)
    routeJ.append(string3)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #7.测试TimeEnrich
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v3":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string3="""{"id":"v3","locations":{"in":"30,40","out":"30,40"},"toNext":{"v2":10},"free":{"openClose":[[40,80]],"durs":[10,20,30]},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[90,100]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    string3=json.loads(string3)
    routeJ.append(string1)
    routeJ.append(string3)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    #8.测试TimeEnrich
    routeJ=list()
    string1="""{"id":"v1","locations":{"in":"23,45","out":"23,45"},"toNext":{"v3":10},"fixed":{"times":[[11,22]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string3="""{"id":"v3","locations":{"in":"30,40","out":"30,40"},"toNext":{"v2":10},"free":{"openClose":[[40,54]],"durs":[10,20,30]},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string2="""{"id":"v2","locations":{"in":"30,35","out":"30,45"},"toNext":"NULL","fixed":{"times":[[90,100]],"unavailable":0,"canDel":0},"arrange":{"controls":{"rangeIdx":0},"time":[],"error":[0,0,0,0,0,0,0]}}"""
    string1=json.loads(string1)
    string2=json.loads(string2)
    string3=json.loads(string3)
    routeJ.append(string1)
    routeJ.append(string3)
    routeJ.append(string2)
    routeJ=json.dumps(routeJ)
#    print routeJ
    routeJ_dic.append(routeJ)

    with io.open('./case.txt','w',encoding='utf-8') as f:
#        result=commands.getoutput('./test '+string1);
        for i,item in enumerate(routeJ_dic):
            f.write(("%s\n"%item).decode('utf-8'))
            #print "%d: %s"%(i,item)
#            result=commands.getoutput('./Ptest '+str(item))

        

main()
    
