#!/usr/bin/python

import requests
import time
from optparse import OptionParser
import sys
import json
import urllib

if __name__ == "__main__":
    ports_legal = set()
    ports_legal.add("91")
    ports_legal.add("92")
    ports_legal.add("9292")
    ports_legal.add("93")
    ports_legal.add("51600")

    parser = OptionParser()
    parser.add_option("-p", "--port",dest="port",help="which port to request", metavar="PORT")
    parser.add_option("-q", "--query",dest="query",help="query file", metavar="QUERY")
    parser.add_option("-o", "--output",dest="output",help="output file", metavar="OUTPUT")
    (options ,args) =parser.parse_args()
    if options.port == None:
        options.port = "92"
    if options.port not in ports_legal:
        print options.port
        print ports_legal
        print "port not legal"
        exit(0)
    if options.query == None:
        options.query = "case";
    if options.output == None:
        options.output = options.query+".resp"
    ip = "10.10.135.140"
    # preb nignx addr
    #ip ="10.19.72.173"
    if options.port == "51600":
        ip = "10.10.169.10"
    with open(options.query,'r') as qf:
        lines = qf.readlines()
        new_line=lines[0].strip('?')
        qbegin = new_line.find('{')
        qend = new_line.rfind('}')
        if qbegin<0 or qend<0:
            print "query already encode"
        elif qbegin>0 and qend>0:
            query = new_line[qbegin:qend+1]
            query_encode = urllib.quote(query)
            new_line=new_line.replace(query,query_encode)
        else:
            print 'bad request!'
            sys.exit(1)
        _params={}
        _t = new_line.split('&')
        new_qid =str(int(round(time.time() * 1000)))
        for i in _t:
            _tt = i.split('=')
            if _tt[0] == "qid":
                _tt[1] = new_qid
            if _tt[0] == "uid":
                _tt[1] = "dataplan"
            if _tt[0] == "query":
                _tt[1] = urllib.unquote(_tt[1])
            if len(_tt) == 2:
                _params[_tt[0]]=_tt[1]
        url="http://"+ip+":"+options.port+"/?"
        r = requests.get(url,params = _params)
        with open(options.output,'w') as of:
            reload(sys)
            sys.setdefaultencoding('utf-8')
            result = list()
            result.append(new_qid)
            result.append("resp:"+str(r.status_code))
            resp = json.loads(r.text)
            result.append(json.dumps(resp,indent=4,ensure_ascii= False))
            result = [ line +"\n" for line in result ]
            of.writelines(result)
