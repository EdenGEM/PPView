#!/usr/bin/python

import requests
import time
import re
from optparse import OptionParser
import sys
import json

if __name__ == "__main__":
    ports_legal = set()
    ports_legal.add("91")
    ports_legal.add("92")
    ports_legal.add("9292")
    ports_legal.add("93")

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
    with open(options.query,'r') as qf:
        lines = qf.readlines()
        new_qid = int(round(time.time() * 1000))
        str_re = "qid="+str(new_qid)
        new_line=re.sub(r'qid=(\d+)',str_re,lines[0])
        url="http://"+ip+":"+options.port+"/"+new_line
        r = requests.get(url)
        with open(options.output,'w') as of:
            reload(sys)
            sys.setdefaultencoding('utf-8')
            result = list()
            result.append(str_re)
            result.append("resp:"+str(r.status_code))
            resp = json.loads(r.text)
            result.append(json.dumps(resp,indent=4,ensure_ascii= False))
            result = [ line +"\n" for line in result ]
            of.writelines(result)


