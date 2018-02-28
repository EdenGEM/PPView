#!/usr/local/bin//python
#-*-coding:UTF-8 -*-
import sys
sys.path.append('../../Common')
from DBHandle import DBHandle
import cgi,cgitb
import re
import urllib2
reload(sys);
exec("sys.setdefaultencoding('utf-8')");

form =cgi.FieldStorage()
qid=form.getvalue('qid')
env=form.getvalue('env')
load=form.getvalue('load')
env_resp="%s_resp"%env
conn=DBHandle('10.10.169.10','root','miaoji1109','viewdiff')
if load=="resp":
    sqlstr="select %s_resp from cmp_cases where %s_qid=%s;"%(env,env,qid)
    resp=conn.do(sqlstr)
    result=resp[0][env_resp]
else:
    sqlstr="select ori_req from cmp_cases where %s_qid=%s"%(env,qid)
    req=conn.do(sqlstr)
    result=req[0]["ori_req"]
    if load=="query":
        pat_query=re.compile(r'&query=([^&]+)')
        match_query=pat_query.search(result)
        if match_query:
           query=match_query.group(1)
           result=urllib2.unquote(query.decode('utf-8','replace').encode('gbk','replace'))
#        url=urllib2.unquote(result.decode('utf-8','replace').encode('gbk','replace'))
#        pat_query=re.compile(r'&query=([^&]+)')
#        match_query=pat_query.search(url)
#        if match_query:
#           query=match_query.group(1)
#           result=query

print """Content-type:text/html\r\n\r\n<html>
	<head>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	</head>
	<body>
	    %s
	</body>
</html>"""%result
