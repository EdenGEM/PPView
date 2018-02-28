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
uid=form.getvalue('uid')
env=form.getvalue('env')
Type=form.getvalue('type')
qid=form.getvalue('qid')
load=form.getvalue('load')
env_resp="%s_resp"%env
conn=DBHandle('10.10.169.10','root','miaoji1109','viewdiff')
if load=="resp":
    sqlstr="select resp from trace_qid where env=%s and uid=%s and type=%s and qid=%s;"%(env,uid,Type,qid)
    resp=conn.do(sqlstr)
    result=resp[0]["resp"]
else:
    sqlstr="select req from trace_qid where env=%s and uid=%s and type=%s and qid=%s;"%(env,uid,Type,qid)
    req=conn.do(sqlstr)
    result=req[0]["req"]
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
