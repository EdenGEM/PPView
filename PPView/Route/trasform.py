#!/usr/local/bin//python

import json
import sys

reload(sys)
sys.setdefaultencoding("utf-8")

files=sys.argv[1]
print files
with open(files) as f:
    lines=f.readlines()
#    resp=json.loads(str(lines))
    result=list()
    a=json.dumps(json.dumps(lines,indent=4,ensure_ascii= False))
    print type(a)
    result.append(a)
    result=[ line + "\n" for line in result ]
    with open('./trans','w') as fp:
        fp.writelines(result)


