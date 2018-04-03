#include "MJLog.h"
// #include "glog/logging.h"
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include "MyTime.h"


using namespace std;
namespace MJ{



int PrintInfo::NEED_DBG_LOG = 1;
int PrintInfo::NEED_STD_LOG = 1;
int PrintInfo::NEED_ERR_LOG = 1;
int PrintInfo::NEED_DUMP_LOG = 1;

pthread_mutex_t MYLog::_locker = PTHREAD_MUTEX_INITIALIZER;
time_t MYLog::_time = 0;
std::ofstream MYLog::_file;
std::string MYLog::_dir = ".";

void logMJOB_RequestClient(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const std::string& req, 
                const size_t len){
    fprintf(stderr, "[RequestClient MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,next_id=%s,req=%s,len=%zd]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), next_id.c_str(), req.c_str(), len);
    return;
}

void logMJOB_ResponseClient(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const std::string& resp, 
                const int len,
                const long& cost){
    fprintf(stderr, "[ResponseClient MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,next_id=%s,resp=%s,len=%d,cost=%ld]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), next_id.c_str(), resp.c_str(), len,cost);
    return;
}


void logMJOB_RequestServer(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& req){
    fprintf(stderr, "[RequestServer MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,req=%s]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), req.c_str());
    return;
}



void logMJOB_ResponseServer(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& resp, 
                const long& cost){
    fprintf(stderr, "[ResponseServer MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,resp=%s,cost=%ld]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), resp.c_str(),cost);
    return;
}


void logMJOB_Exception(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& debug){
    fprintf(stderr, "[Exception MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,debug=%s]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), debug.c_str());
    return;
}


void logMJOB_Reload(const long& ts,
                const std::string& ip,
                const int error_id,
                const long& cost,
                const std::string& debug){
    fprintf(stderr, "[Reload MJOPObserver,type=reload,qid=%ld,ts=%ld,ip=%s,error_id=%d,cost=%ld,debug=%s]\n",ts, ts, ip.c_str(), error_id, cost, debug.c_str());
    return;
}

void logMJOB_MSGSend(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const std::string& req){
    fprintf(stderr, "[MSGSend MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,next_id=%s,req=%s]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), next_id.c_str(), req.c_str());
    return;
}

void logMJOB_MSGRecv(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const int len){
    fprintf(stderr, "[MSGRecv MJOPObserver,type=%s,uid=%s,csuid=%s,qid=%s,ts=%ld,ip=%s,refer_id=%s,cur_id=%s,next_id=%s,len=%d]\n",type.c_str(), uid.c_str(), csuid.c_str(), qid.c_str(), ts, ip.c_str(), refer_id.c_str(), cur_id.c_str(), next_id.c_str(), len);
    return;
}




void MYLog::init(const std::string& dir){
	time_t t;
	time(&t);
	string time_str = MyTime::toString(t,8);
	_dir = dir;
	if (_dir[_dir.length()-1]=='/')
		_dir = _dir.substr(0,_dir.length()-1);
	_time = MyTime::toTime(time_str,8);
	_file.open((_dir+"/"+time_str.substr(0,11)+".txt").c_str(),ios::app|ios::out);
	cerr<<_dir+"/"+time_str.substr(0,11)+".txt"<<endl;
	if (!_file){
		cerr<<"Log file open failed!"<<endl;
		exit(0);
	}
	return;
}
void MYLog::write(const std::string& log){
	pthread_mutex_lock(&_locker);
	time_t t;
	time(&t);
	if (t>_time+3599){
		if (_time){
			_file.close();
		}
		_time += 3600*((t-_time)/3600);
		string hour_str = MyTime::toString(_time,8);
		_file.open((_dir+"/"+hour_str+".txt").substr(0,11).c_str(),ios::app|ios::out);
		if (!_file){
			cerr<<"Log file open failed!"<<endl;
			exit(0);
		}
	}
	_file<<"["<<MyTime::toString(t,8,"%Y%m%d_%H:%M:%S")<<"]"<<log<<std::endl;
	pthread_mutex_unlock(&_locker);
	return;
}
void MYLog::write(const int& log){
	pthread_mutex_lock(&_locker);
	time_t t;
	time(&t);
	if (t>_time+3599){
		if (_time){
			_file.close();
		}
		_time += 3600*((t-_time)/3600);
		string hour_str = MyTime::toString(_time,8);
		_file.open((_dir+"/"+hour_str+".txt").substr(0,11).c_str(),ios::app|ios::out);
		if (!_file){
			cerr<<"Log file open failed!"<<endl;
			exit(0);
		}
	}

	_file<<"["<<MyTime::toString(t,8,"%Y%m%d_%H:%M:%S")<<"]"<<log<<std::endl;
	pthread_mutex_unlock(&_locker);
	return;
}

int PrintInfo::SwitchDbg(int need_dbg, int need_log, int need_err, int need_dump) {
    NEED_DBG_LOG = need_dbg;
    NEED_STD_LOG = need_log;
    NEED_ERR_LOG = need_err;
    NEED_DUMP_LOG = need_dump;
    return 0;
}

int PrintInfo::PrintLog(const char* format, ...) {
    if (!NEED_STD_LOG) return 1;
    time_t t;
    time(&t);
    char buff[MAXLOGLEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, MAXLOGLEN, format, args);
    va_end(args);
    //		fprintf(stderr, "[MIOJI][LOG][%s]%s\n", MyTime::toString(t,8,"%Y%m%d_%H:%M:%S").c_str(), buff);
    fprintf(stderr, "[LOG][%s]%s\n", MyTime::toString(t,8,"%m%d_%H:%M:%S").c_str(), buff);
    return 0;
}


int PrintInfo::PrintDbg(const char* format, ...) {
    if (!NEED_DBG_LOG) return 1;
    time_t t;
    time(&t);
    char buff[MAXLOGLEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, MAXLOGLEN, format, args);
    va_end(args);
    //		fprintf(stderr, "[MIOJI][DBG][%s]%s\n", MyTime::toString(t,8,"%Y%m%d_%H:%M:%S").c_str(), buff);
    fprintf(stderr, "[DBG][%s]%s\n", MyTime::toString(t,8,"%m%d_%H:%M:%S").c_str(), buff);
    return 0;
}


int PrintInfo::PrintErr(const char* format, ...) {
    if (!NEED_ERR_LOG) return 1;
    time_t t;
    time(&t);
    char buff[MAXLOGLEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, MAXLOGLEN, format, args);
    va_end(args);
    //		fprintf(stderr, "[MIOJI][ERR][%s]%s\n", MyTime::toString(t,8,"%Y%m%d_%H:%M:%S").c_str(), buff);
    fprintf(stderr, "[ERR][%s]%s\n", MyTime::toString(t,8,"%m%d_%H:%M:%S").c_str(), buff);
    return 0;
}





}//namespace MJ
