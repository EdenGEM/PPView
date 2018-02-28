> 使用：
> 批量对比：eg：python run.py -d 20170919 -t s127 -n 10 -e test/offline -m caoxiaolan@mioji.com >a
	  注意：1.默认是从运维库中的 test 环境拿请求 ，-e 指定一个或两个除 online 以外的环境 （不打online环境）2. 指定一个环境时，默认是和运维库中的 test 的 response对比，若指定了两个环境，则对比的是新打的请求的response 3.可用-s 参数（-s bug）来指定从error_qid 表中拿请求和response,从该表中拿数据需要指定环境，所以共需要指定两个环境，此时只会新打第二个环境，然后与error_qid 表中查询的response对比
	
> 指定qid对比：eg：python run.py -q 1505979312144 -t s127 -e test -m caoxiaolan@mioji.com >a 

# class ResCmp: 
## public:
> ResCmp.py
    Options:
  -h, --help            show this help message and exit
  -d DATE, --date=DATE  run date like 20150101
  -t TYPE, --type=TYPE  run type like s125
  -n NUM, --num=NUM     run case number like 100
  -q QID, --qid=QID     
  -x CMP1, --cmp1=CMP1  the first cmp_env or source_env
  -y CMP2, --cmp2=CMP2  the second cmp_env or dest_env
  -m MAIL, --mail=MAIL  mail to like caoxiaolan@mioji.com


## private:

> //从运维数据库拉取请求和相应

> LoadReqAndRespByPatch(kDate,kType,kNum);  // for patch
    Load(kDate,kType,kNum,kQid,Senv)    //从online库中拿req
    Query(qid,env,req)                  //打请求
    Store(kDate,kType)                  //四个环境的resp存库

> LoadReqAndRespByQid(kQid,kSource_env,kDest_env,Mail);    //for qid
    Query(qid,env,req)

## private:

> //比较函数
> CmpInfoByPatch(kDate,kType,kNum,kCmp1,kCmp2,Mail)
    Fetch(kCmp1,kCmp2,kType,kDate,kNum)  //拿库中相应环境的响应

> CmpInfoByQid(kType,qid,cmp2_qid,resp1,resp2,kcmp1,kcmp2,mail)

> CmpInfo(cmp1resp,cmp2resp,kCmp1,kCmp2);

> CmpProduct(cmp1resp,cmp2resp,kCmp1,kCmp2);

> CmpView(cmp1_view,cmp2_view,cidx)


## private:

> // 发邮件相关

    mymail.Report(qid_dic,kCmp1,kCmp2)



