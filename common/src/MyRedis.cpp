#include "MyRedis.h"
#include <iostream>
#include <sstream>

using namespace std;
using namespace MJ;

redisReply* MyRedis::doCMD(const char* cmd){

	struct timeval _start;
	struct timeval _end;
	gettimeofday(&_start, NULL);

	redisReply* r = NULL;
	if (_redis_cnn)
		r = (redisReply*)redisCommand(_redis_cnn,cmd);


	gettimeofday(&_end, NULL);
	long cost = (_end.tv_sec-_start.tv_sec)*1000000+(_end.tv_usec-_start.tv_usec);
	if(cost >= max_redis_cost_time)
	{
		fprintf(stderr, "[Debug MiojiOPObserver,debuginfo=\"redis cmd cost overtime\",threshold=%d,cost=%ld]\n",max_redis_cost_time, cost);
		fprintf(stderr, "cost overtime cmd len == %zu\n", strlen(cmd));
		fprintf(stderr, "cost overtime cmd == ");
		for(int i=0; i<10 && cmd[i]!='\0'; ++i)
			fprintf(stderr, "%c", cmd[i]);
		fprintf(stderr, "\n redis addr == %s\n", _addr.c_str());
	}
	return r;
}

bool MyRedis::set_expire(const std::string& key,size_t expire_seconds)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"expire %s %d",key.c_str(),expire_seconds);
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"expire %s %d",key.c_str(),expire_seconds);
		}
		else
		{
			cerr<<"[MyRedis::set_expire] failed->[" << key << "]"<<endl;
			return false;
		}
	}


	if (r==NULL){
		cerr << "[MyRedis::set_expire 2] failed->" << key << endl;
		return false;
	}

	if (!(r->type == REDIS_REPLY_INTEGER && 1 == r->integer))
	{
		cerr << "[MyRedis::set_expire 3] failed->" << key << endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;
}

bool MyRedis::set_value_and_expire(const std::string& key, const std::string& val,const size_t& ttl_time)
{
	bool set_key = set(key,val);
	if (false == set_key)
	{
		return false;
	}

	bool set_expire_flag = set_expire(key,ttl_time);

	return set_expire_flag;
}

int MyRedis::setnx_and_expire(const std::string& key, const std::string& val,const size_t& ttl_time)
{
	int setnx_ret = setnx(key,val);
	if (-1 == setnx_ret)
		return -1;

	if (1==setnx_ret && !set_expire(key,ttl_time))
	{
		del(key);
		return -1;
	}

	return setnx_ret;
}

int MyRedis::setnx(const std::string& key,const std::string& val)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"setnx %s %b",key.c_str(),val.c_str(),val.size());
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"setnx %s %b",key.c_str(),val.c_str(),val.size());
		}
		else
		{
			cerr<<"[MyRedis::setnx] failed->[" << key << "]"<<endl;
			return -1;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::setnx] failed->"<<key<<endl;
		return -1;
	}

	if(r->type == REDIS_REPLY_INTEGER)
	{
		int ret = r->integer;
		freeReplyObject(r);
		return ret;
	}
	else
	{
		cerr<<"[MyRedis::setnx] failed->"<<key<<endl;
		freeReplyObject(r);
		return -1;
	}

	freeReplyObject(r);
	return true;
}

bool MyRedis::set(const std::string& key,const std::string& val)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"set %s %b",key.c_str(),val.c_str(),val.size());
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"set %s %b",key.c_str(),val.c_str(),val.size());
		}
		else
		{
			cerr<<"[MyRedis::set] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::set] failed->"<<key<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::set] failed->"<<key<<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;
}


bool MyRedis::flushDb()
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"flushall");
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"flushall");
		}
		else
		{
			cerr<<"[MyRedis::flushdb] failed ]" << endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::flushdb] failed]" << endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::flushdb failed] [" << string(r->str) << "]"<< endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;
}



bool MyRedis::set_old(const std::string& key,const std::string& val)
{
	const string cand_cmd = "set " + key + " " + val;
	cerr << "cmd: " << cand_cmd << endl;
	redisReply* r = doCMD(cand_cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cand_cmd.c_str());
		}else{
			cerr << "ca 1\n";
			cerr<<"[MyRedis::set] failed->"<<key<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::set] failed->"<<key<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::set] failed->"<<key<<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;
}


bool MyRedis::get(const std::vector<std::string>& keys,tr1::unordered_map<std::string,std::string>& vals)
{
	if(keys.size()==0)
		return false;
	string cmd = "mget";
	vals.clear();
	size_t i;
	for (i=0;i<keys.size();i++){
		if (keys[i].length()==0)
			cmd += (" ThisIsAImpossibleRedisKey");
		else
			cmd += (" "+keys[i]);
	}
	redisReply* r = NULL;
	if(_redis_cnn)
		r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::mget][1] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::mget][2] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::mget] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->elements > keys.size())
	{
		cerr << "[MyRedis::mget] r->elements > keys.size()" << endl;
		freeReplyObject(r);
		return false;
	}
	for (i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING){
			vals[keys[i]]=string(childReply->str,childReply->len);
		}else if (childReply->type == REDIS_REPLY_NIL){
			vals[keys[i]]="";
		}
#if REDIS_DEBUG
		//fprintf(stderr,"[MyRedis][get][key=%s][val=%s]\n",keys[i].c_str(),vals[keys[i]].c_str());
		std::cerr << "[debug][MyRedis][get][key=" << keys[i] << "][val=" << vals[keys[i]] << "]" << std::endl;
#endif
	  }

	freeReplyObject(r);

	return true;
}

bool MyRedis::get(const std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	if(keys.size()==0)
		return false;
	string cmd = "mget";
	vals.clear();
	size_t i;
	for (i=0;i<keys.size();i++){
		if (keys[i].length()==0)
			cmd += (" ThisIsAImpossibleRedisKey");
		else
			cmd += (" "+keys[i]);
	}

	redisReply* r = NULL;
	if(_redis_cnn)
		r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::mget] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::mget] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::mget] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->elements > keys.size())
	{
		cerr << "[MyRedis::mget] r->elements size > keys.size()" << endl;
		return false;
	}
	else if(r->elements < keys.size())
	{
		cerr << "[MyRedis::mget] r->elements size < keys.size()" << endl;
		return false;
	}
	for (i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING){
			vals.push_back(string(childReply->str,childReply->len));
		}else if (childReply->type == REDIS_REPLY_NIL){
			vals.push_back("");
		}
#if REDIS_DEBUG
		//fprintf(stderr,"[MyRedis][get][key=%s][val=%s]\n",keys[i].c_str(),vals[i].c_str());
		std::cerr << "[debug MyRedis][get][key=" << keys[i] << "][val=" << vals[i] << "]" << std::endl;
#endif
	}

	freeReplyObject(r);

	if (vals.size()!=keys.size()){
		vals.clear();
		cerr<<"[ERROR]:MyRedis::mget() keys' size not match vals'->"<<cmd<<endl;
	}
	return true;
}

bool MyRedis::get(const std::string& key,std::string& val){

	val = "";
	if (key.length()==0){
		cerr<<"null key"<<endl;
		return false;
	}

	redisReply* r = NULL;

	if(_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"get %s ",key.c_str());
	}

	if (r==NULL)
	{
		if (reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"get %s ",key.c_str());
		}
		else
		{
			cerr<<"[MyRedis::get] failed"<<endl;
			return false;
		}
	}

	if (r==NULL)
	{
		cerr<<"[MyRedis::get] failed"<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_STRING){
		if (r->type != REDIS_REPLY_NIL){
			cerr<<"[MyRedis::get] failed"<<endl;
			freeReplyObject(r);
			return false;
		}else{
			//cerr << "[MyRedis::get] return not string, r->type = " << r->type << endl;
			freeReplyObject(r);
			return true;
		}
	}

	val = string(r->str,r->len);

	freeReplyObject(r);
	return true;
}

bool MyRedis::ttl(const std::string &key, int *ttl_time)
{
	if (key.length()==0){
		cerr<<"null key"<<endl;
		return false;
	}

	redisReply *r = NULL;
	if(_redis_cnn)
		r = doCMD(("TTL "+key).c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(("TTL "+key).c_str());
		}else{
			cerr<<"[MyRedis::ttl] failed->"<<key<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::ttl] failed->"<<key<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_INTEGER){
		if (r->type != REDIS_REPLY_NIL){
			cerr<<"[MyRedis::ttl] failed->"<<key<<endl;
			freeReplyObject(r);
			return false;
		}else{
			freeReplyObject(r);
			return true;
		}
	}

	*ttl_time = r->integer;
	freeReplyObject(r);
	return true;
}

MyRedis::MyRedis(){
	_redis_cnn = NULL;
}
MyRedis::~MyRedis(){
	if (_redis_cnn){
		redisFree(_redis_cnn);
		_redis_cnn = NULL;
	}
}
MyRedis::MyRedis(MyRedis* m_redis)
{
    _redis_cnn = NULL;
    init(m_redis->_addr, m_redis->_password, m_redis->_db);
}

bool MyRedis::reConnect(){
	//cerr << "Connecting MyRedis" << endl;
	if (_redis_cnn){
		redisFree(_redis_cnn);
		_redis_cnn = NULL;
	}
	//_redis_cnn = redisConnectUnix("/search/redis/redis.sock");

	_redis_cnn = redisConnect(_ip.c_str(), _port);
	//_redis_cnn = select_db(_redis_cnn);

	if (_redis_cnn==NULL){
		cerr<<"[ERROR]:Redis Connect return NULL"<<endl;
		return false;
	}
	if (_redis_cnn->err){
		cerr<<"[ERROR]:Redis Connect failed [" << _redis_cnn->err << "] [" << _redis_cnn->errstr << "]"<<endl;
		redisFree(_redis_cnn);
		_redis_cnn = NULL;
		return false;
	}

	//auth by passwd
	if("" != _password)
	{
	redisReply* reply =(redisReply*) redisCommand(_redis_cnn,"AUTH %s",_password.c_str());

	if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
	{
		cerr  << "[ERROR]: Redis Auth Fail! password is :" << _password << endl;
		return false;
	}
	}
	_redis_cnn = select_db(_redis_cnn);
	return true;
}
bool MyRedis::init(const std::string& addr,const std::string& passwd,const int db)
{
	size_t pos;
	if ((pos=addr.find(":"))==std::string::npos){
		cerr<<"[ERROR]:MyRedis::init() error addr->"<<addr<<endl;
		return false;
	}
    //cout << addr << endl;
    //cout << pos << endl;
	_addr = addr;
	_ip = addr.substr(0,pos);
	_port = atoi(addr.substr(pos+1).c_str());
	_password = passwd;
	_db = db;

	return reConnect();
}
bool MyRedis::hget(const std::string& hkey,const std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	string cmd = "hmget "+hkey;
	vals.clear();
	size_t i;
	for (i=0;i<keys.size();i++){
		cmd += (" "+keys[i]);
	}

	redisReply* r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::hmget] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::hmget] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::hmget] failed"<<endl;
		freeReplyObject(r);
		return false;
  }
  for (i=0;i<r->elements;i++){
  	redisReply* childReply = r->element[i];
  	if (childReply->type == REDIS_REPLY_STRING){
  		vals.push_back(childReply->str);
  	}else if (childReply->type == REDIS_REPLY_NIL){
  		vals.push_back("");
  	}
  }

	freeReplyObject(r);

	if (vals.size()!=keys.size()){
  	vals.clear();
  	cerr<<"[ERROR]:MyRedis::hmget() keys' size not match vals'->"<<cmd<<endl;
  }
	return true;
}

bool MyRedis::hgetall(const std::string& hkey,std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	string cmd = "hgetall "+hkey;
	keys.clear();
	vals.clear();

	redisReply* r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::hgetall] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::hgetall] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::hgetall] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->elements%2!=0){
		cerr<<"[MyRedis::hgetall] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
  for (size_t i=0;i<r->elements;i+=2){
  	redisReply* childReply = NULL;
	childReply = r->element[i];
  	if (childReply->type == REDIS_REPLY_STRING){
  		keys.push_back(childReply->str);
  	}else if (childReply->type == REDIS_REPLY_NIL){
  		keys.push_back("");
  	}
	childReply = r->element[i+1];
  	if (childReply->type == REDIS_REPLY_STRING){
  		vals.push_back(childReply->str);
  	}else if (childReply->type == REDIS_REPLY_NIL){
  		vals.push_back("");
  	}
  }

	freeReplyObject(r);

	if (vals.size()!=keys.size()){
  	vals.clear();
  	cerr<<"[ERROR]:MyRedis::hgetall() keys' size not match vals'->"<<cmd<<endl;
  	}
	return true;
}

/*bool MyRedis::hmset(const std::string& key,const std::map<std::string, std::string>& value_map)
{
	string cmd_args = "hmset " + key;
	for(map<string,string>::const_iterator it = value_map.begin(); it != value_map.end(); it++)
	{
		//cmd_args += " "+it->first+" '"+it->second+"'";
	        cmd_args += " "+it->first+" "+it->second;
	}

	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn, cmd_args.c_str());
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn, cmd_args.c_str());
		}
		else
		{
			cerr<<"[MyRedis::hmset1] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::hmset2] failed"<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::hmset3] failed, info is "<< r->str <<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;

}*/
bool MyRedis::mset(const std::map<std::string, std::string>& key_value_map)
{
	vector<const char *> argv;
	vector<size_t> argvlen;
	static char cmd[] = "MSET";
	argv.push_back( cmd );
	argvlen.push_back( sizeof(cmd)-1 );

	for(map<string,string>::const_iterator it = key_value_map.begin(); it != key_value_map.end(); it++)
	{
		//cmd_args += " "+it->first+" '"+it->second+"'";
	        //cmd_args += " "+it->first+" "+it->second;
		argv.push_back(it->first.c_str());
		argv.push_back(it->second.c_str());
		argvlen.push_back(it->first.size());
		argvlen.push_back(it->second.size());
	}

	redisReply* r = NULL;
	if (_redis_cnn)
	{
//		cerr<<"commond is hmset " << cmd_args.c_str() << endl;
		r = (redisReply*)redisCommandArgv(_redis_cnn, argv.size(),&(argv[0]), &(argvlen[0]));
	}

	if (NULL == r)
	{
		if(reConnect())
		{
		//	r = (redisReply*)redisCommand(_redis_cnn,"hmset %s", cmd_args.c_str());
			r = (redisReply*)redisCommandArgv(_redis_cnn, argv.size(),&(argv[0]), &(argvlen[0]));
		}
		else
		{
			cerr<<"[MyRedis::mset1] failed ]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::mset2] failed"<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::mset3] failed, info is "<< r->str <<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;

}

bool MyRedis::hmset(const std::string& key,const std::map<std::string, std::string>& value_map)
{
	string cmd_args = key;
	vector<const char *> argv;
	vector<size_t> argvlen;
	static char cmd[] = "HMSET";
	argv.push_back( cmd );
	argvlen.push_back( sizeof(cmd)-1 );

	argv.push_back( key.c_str() );
	argvlen.push_back( key.size() );

	for(map<string,string>::const_iterator it = value_map.begin(); it != value_map.end(); it++)
	{
		//cmd_args += " "+it->first+" '"+it->second+"'";
	        //cmd_args += " "+it->first+" "+it->second;
		argv.push_back(it->first.c_str());
		argv.push_back(it->second.c_str());
		argvlen.push_back(it->first.size());
		argvlen.push_back(it->second.size());
	}

	redisReply* r = NULL;
	if (_redis_cnn)
	{
//		cerr<<"commond is hmset " << cmd_args.c_str() << endl;
		r = (redisReply*)redisCommandArgv(_redis_cnn, argv.size(),&(argv[0]), &(argvlen[0]));
	}

	if (NULL == r)
	{
		if(reConnect())
		{
		//	r = (redisReply*)redisCommand(_redis_cnn,"hmset %s", cmd_args.c_str());
			r = (redisReply*)redisCommandArgv(_redis_cnn, argv.size(),&(argv[0]), &(argvlen[0]));
		}
		else
		{
			cerr<<"[MyRedis::hmset1] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::hmset2] failed"<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::hmset3] failed, info is "<< r->str <<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;

}
bool MyRedis::hmset_value_and_expire(const std::string& key, const std::map<std::string,std::string>& value_map, const size_t& ttl_time)
{
	if(!hmset(key, value_map))
	{
		return false;
	}
	if(!set_expire(key, ttl_time))
	{
		return false;
	}
	return true;
}

bool MyRedis::scan(const unsigned long& cursor, unsigned long& next_cursor, std::vector<std::string> keys_vec)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"scan %lu",cursor);
	}

	if(NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"scan %lu",cursor);
		}
		else
		{
			cerr<<"[MyRedis::scan] failed, cursor is [" << cursor << "]"<<endl;
			return false;
		}
	}

	if(r==NULL)
	{
		cerr<<"[MyRedis::scan 2] failed, cursor is [" << cursor << "]"<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_ARRAY || r->elements != 2)
	{
		cerr<<"[MyRedis::scan] failed"<<endl;
		freeReplyObject(r);
		return false;
	}

	redisReply* cursorReply = r->element[0];
	if(cursorReply->type == REDIS_REPLY_STRING)
	{
		string cursor_str = string(cursorReply->str,cursorReply->len);
		next_cursor = strtoul(cursor_str.c_str(),NULL,10);
	}
	else
	{
		cerr<<"[MyRedis::scan] failed, return value error"<<endl;
		freeReplyObject(r);
		return false;
	}

	redisReply* keyReply = r->element[1];
	if(keyReply->type == REDIS_REPLY_ARRAY)
	{
		for(size_t i = 0; i<keyReply->elements; ++i)
		{
			redisReply* tmpReply = keyReply->element[i];
			if(tmpReply->type == REDIS_REPLY_STRING)
			{
				string redis_key = string(tmpReply->str,tmpReply->len);
				keys_vec.push_back(redis_key);
			}
		}
	}
	else
	{
		cerr<<"[MyRedis::scan 2] failed, return value error"<<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;


}

bool MyRedis::scan(const unsigned long& cursor,const int& count, unsigned long& next_cursor, std::vector<std::string>& keys_vec)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"scan %lu count %d",cursor,count);
	}

	if(NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"scan %lu count %d",cursor,count);
		}
		else
		{
			cerr<<"[MyRedis::scan] failed, cursor is [" << cursor << "]"<<endl;
			return false;
		}
	}

	if(r==NULL)
	{
		cerr<<"[MyRedis::scan 2] failed, cursor is [" << cursor << "]"<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_ARRAY || r->elements != 2)
	{
		cerr<<"[MyRedis::scan] failed"<<endl;
		freeReplyObject(r);
		return false;
	}

	redisReply* cursorReply = r->element[0];
	if(cursorReply->type == REDIS_REPLY_STRING)
	{
		string cursor_str = string(cursorReply->str,cursorReply->len);
		next_cursor = strtoul(cursor_str.c_str(),NULL,10);
	}
	else
	{
		cerr<<"[MyRedis::scan] failed, return value error"<<endl;
		freeReplyObject(r);
		return false;
	}

	redisReply* keyReply = r->element[1];
	if(keyReply->type == REDIS_REPLY_ARRAY)
	{
		for(size_t i = 0; i<keyReply->elements; ++i)
		{
			redisReply* tmpReply = keyReply->element[i];
			if(tmpReply->type == REDIS_REPLY_STRING)
			{
				string redis_key = string(tmpReply->str,tmpReply->len);
				keys_vec.push_back(redis_key);
			}
		}
	}
	else
	{
		cerr<<"[MyRedis::scan 2] failed, return value error"<<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;


}

bool MyRedis::keys(const std::string& key, std::vector<std::string>& valkeys)
{
	string cmd = "keys "+key;
	valkeys.clear();

	redisReply* r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::keys] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::keys] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::keys] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	for (size_t i=0;i<r->elements;i++){
		redisReply* childReply = NULL;
		childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING){
			valkeys.push_back(childReply->str);
		}
	}

	freeReplyObject(r);

	return true;
}

bool MyRedis::del(const string& key)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"del %s",key.c_str());
	}

	if (NULL == r)
	{
		if (reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"del %s",key.c_str());
		}
		else
		{
			cerr<<"[MyRedis::del] failed->[" << key << "]" << endl;
			return false;
		}
	}

	if (r == NULL)
	{
		return false;
	}

	if(r->type != REDIS_REPLY_INTEGER)
	{
		if (r->type != REDIS_REPLY_NIL)
		{
			freeReplyObject(r);
			return false;
		}
		else
		{
			freeReplyObject(r);
			return true;
		}

	}

	freeReplyObject(r);
	return true;
}

redisContext* MyRedis::select_db(redisContext *c)
{
	redisReply *reply;
	reply = (redisReply*)redisCommand(c,"SELECT %d", _db);

	if(reply != NULL)
	{
		freeReplyObject(reply);
	}

	return c;
}

/*
bool MyRedis::del(const string& key)
{
	string cmd = "del "+key;

	redisReply* r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::del] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::del] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_INTEGER){
		if (r->type != REDIS_REPLY_NIL){
			cerr<<"[MyRedis::del] failed"<<endl;
			freeReplyObject(r);
			return false;
		}else{
			freeReplyObject(r);
			return true;
		}
	}

	freeReplyObject(r);

	return true;
}
*/

bool MyRedis::mset(const std::string& key_value)
{
	//cout << "key_value=" << key_value << endl;
	string res = "mset " + key_value;
	//cout << "res=" << res << endl;
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		//r = (redisReply*)redisCommand(_redis_cnn,"mset %s",key_value.c_str());
		//r = (redisReply*)redisCommand(_redis_cnn,"mset 1 a 3 b");
		r = (redisReply*)redisCommand(_redis_cnn, res.c_str());
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"mset %s",key_value.c_str());
		}
		else
		{
			//cerr<<"[MyRedis::mset1] failed->[" << key_value << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::mset2] failed"<<endl;
		//cerr<<"[MyRedis::mset2] failed->"<<key_value<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		//cerr << r->str << endl;
		//cerr<<"[MyRedis::mset3] failed->"<<key_value<<endl;
		cerr<<"[MyRedis::mset3] failed"<<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;
}

bool MyRedis::sinter(const std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	if(keys.size()==0)
		return false;
	string cmd = "sinter";
	vals.clear();
	size_t i;
	for (i=0;i<keys.size();i++){
		cmd += (" "+keys[i]);
	}

	redisReply* r = NULL;
	if(_redis_cnn)
		r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::sinter] reconnect failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::sinter] failed, r==NULL"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::sinter] failed, r->type != REDIS_REPLY_ARRAY"<<endl;
		freeReplyObject(r);
		return false;
	}
	for (i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING){
			vals.push_back(string(childReply->str,childReply->len));
		}else if (childReply->type == REDIS_REPLY_NIL){
			vals.push_back("");
		}
#if REDIS_DEBUG
		//fprintf(stderr,"[MyRedis][get][key=%s][val=%s]\n",keys[i].c_str(),vals[i].c_str());
		//std::cerr << "[debug MyRedis][sinter][key=" << keys[i] << "][val=" << vals[i] << "]" << std::endl;
#endif
	}

	freeReplyObject(r);

	return true;
}

bool MyRedis::sunion(const std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	if(keys.size()==0)
		return false;
	string cmd = "sunion";
	vals.clear();
	size_t i;
	for (i=0;i<keys.size();i++){
		cmd += (" "+keys[i]);
	}

	redisReply* r = NULL;
	if(_redis_cnn)
		r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::sunion] reconnect failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::sunion] failed, r==NULL"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::sunion] failed, r->type != REDIS_REPLY_ARRAY"<<endl;
		freeReplyObject(r);
		return false;
	}
	for (i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING){
			vals.push_back(string(childReply->str,childReply->len));
		}else if (childReply->type == REDIS_REPLY_NIL){
			vals.push_back("");
		}
#if REDIS_DEBUG
		//fprintf(stderr,"[MyRedis][get][key=%s][val=%s]\n",keys[i].c_str(),vals[i].c_str());
		//std::cerr << "[debug MyRedis][sinter][key=" << keys[i] << "][val=" << vals[i] << "]" << std::endl;
#endif
	}

	freeReplyObject(r);

	return true;
}

bool MyRedis::isConnected()
{
	return _redis_cnn!=NULL;
}
bool MyRedis::smembers(const std::string& key, std::vector<std::string>& vals)
{
	string cmd = "smembers " + key;
	vals.clear();

	redisReply* r = NULL;
	if(_redis_cnn)
		r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::smembers][1] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::smembers][2] failed"<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::smembers][3] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	for (size_t i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING)
			vals.push_back(childReply->str);
	}

	freeReplyObject(r);
	return true;
}

/*
 * 功能:批量的将元素加入到set集合中
 *
 * 参数1: redis的key
 * 参数2: 所有要插入到set集合中的元素,每个元素之间用空格分割开
 *
 * */
bool MyRedis::sadd(const std::string& key,const string& value_str_sep_by_space)
{
	redisReply* r = NULL;

	const string cmd_str = "sadd " + key + " " + value_str_sep_by_space;

	if(_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn, cmd_str.c_str());
	}

	if (NULL == r)
	{
		if (reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn, cmd_str.c_str());
		}
		else
		{
			cerr << "[MyRedis::sadd reConnect failed]\n";
			return false;
		}
	}

	if (NULL == r)
	{
		cerr << "[MyRedis::sadd] failed-> " << key << endl;
		return false;
	}

	if (r->type != REDIS_REPLY_INTEGER)
	{
		cerr << "[MyRedis::sadd] failed-> " << key << endl;
		cerr << "error type: " << r->type << endl;
		freeReplyObject(r);

		return false;
	}

	freeReplyObject(r);
	return true;
}

/*
 * 功能:批量的将元素加入到set集合中
 *
 * 参数1: redis的key
 * 参数2: 所有要插入到set集合中的元素
 *
 * */
bool MyRedis::sadd(const std::string& key,const std::tr1::unordered_set<std::string>& value_set)
{
	ostringstream oss;
	const int each_value_count = 5000;
	
	int count = 0;
	for(std::tr1::unordered_set<std::string>::const_iterator val_it = value_set.begin();val_it != value_set.end();val_it++)
	{
		count++;

		if (0 == count % each_value_count)
		{
			const string value_str_sep_by_space = oss.str();
			bool add_val = sadd(key,value_str_sep_by_space);

			if (false == add_val)
			{
				cerr << "[MyRedis::sadd fail]\n";
				return false;
			}

			oss.str("");
		}
		else
		{
			oss << " " << *val_it;
		}
	}

	const string value_str_sep_by_space = oss.str();

	if ("" != value_str_sep_by_space)
	{
		bool add_val = sadd(key,value_str_sep_by_space);
		if (false == add_val)  
		{
			cerr << "[MyRedis::sadd fail]\n";
			return false;
		}
	}

	return true;
}

/*
 * 功能:批量的移除set集合中的元素
 *
 * 参数1: redis的key
 * 参数2: 所有要移除的元素,每个元素之间用空格隔开
 *
 * */
bool MyRedis::srem(const std::string& key,const std::string& value_str_sep_by_space)
{
	redisReply* r = NULL;

	const string cmd_str = "srem " + key + " " + value_str_sep_by_space;

	if(_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn, cmd_str.c_str());
	}

	if (NULL == r)
	{
		if (reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn, cmd_str.c_str());
		}
		else
		{
			cerr << "[MyRedis::srem reConnect failed]\n";
			return false;
		}
	}

	if (NULL == r)
	{
		cerr << "[MyRedis::srem] failed-> " << key << endl;
		return false;
	}

	if (r->type != REDIS_REPLY_INTEGER)
	{
		cerr << "[MyRedis::srem] failed-> " << key << endl;
		cerr << "error type: " << r->type << endl;
		freeReplyObject(r);

		return false;
	}

	freeReplyObject(r);
	return true;
}

/*
 * 功能:批量的移除set集合中的元素
 *
 * 参数1: redis的key
 * 参数2: 所有要移除的元素
 *
 * */
bool MyRedis::srem(const std::string& key,const std::tr1::unordered_set<std::string>& value_set)
{
	ostringstream oss;
	const int each_value_count = 5000;
	
	int count = 0;
	for(std::tr1::unordered_set<std::string>::const_iterator val_it = value_set.begin();val_it != value_set.end();val_it++)
	{
		count++;

		if (0 == count % each_value_count)
		{
			const string value_str_sep_by_space = oss.str();
			bool del_val = srem(key,value_str_sep_by_space);

			if (false == del_val)
			{
				cerr << "[MyRedis::srem fail]\n";
				return false;
			}

			oss.str("");
		}
		else
		{
			oss << " " << *val_it;
		}
	}

	const string value_str_sep_by_space = oss.str();

	if ("" != value_str_sep_by_space)
	{
		bool del_val = srem(key,value_str_sep_by_space);
		if (false == del_val)  
		{
			cerr << "[MyRedis::srem fail]\n";
			return false;
		}
	}

	return true;
}

/*
 * 功能:获取集合中的元素的数量
 *
 * 参数1: redis的key
 * 参数2: 集合中元素的数据
 *
 * */
bool MyRedis::scard(const std::string& key,int& val_count)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"scard %s",key.c_str());
	}

	if (NULL == r)
	{
		if (reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"scard %s",key.c_str());
		}
		else
		{
			cerr<<"[MyRedis::del] failed->[" << key << "]" << endl;
			return false;
		}
	}

	if (r == NULL)
	{
		return false;
	}

	if(r->type != REDIS_REPLY_INTEGER)
	{
		freeReplyObject(r);
		return false;
	}
	val_count = r->integer;

	freeReplyObject(r);
	return true;
}

/*
 * 功能:将值value关联到key，并设置key的生存时间
 *
 * 参数1: redis的key
 * 参数2: redis的value
 * 参数3: key的生存时间,单位为秒
 *
 * */
bool MyRedis::setex(const std::string& key,const std::string& val,const int expire_seconds)
{
	return set_value_and_expire(key, val, expire_seconds);
}

/*
 * 功能:获取集合中的所有元素
 *
 * 参数1: redis的key
 * 参数2: 集合中的所有元素
 *
 * */
bool MyRedis::smembers(const string& key,tr1::unordered_set<string>& val_set)
{
	string cmd = "smembers " + key;
	val_set.clear();

	redisReply* r = NULL;
	if(_redis_cnn)
		r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::smembers][1] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::smembers][2] failed"<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::smembers][3] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	for (size_t i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING)
			val_set.insert(childReply->str);
	}

	freeReplyObject(r);
	return true;
}

bool MyRedis::get(vector<string>::const_iterator beg_it,vector<string>::const_iterator end_it,tr1::unordered_map<string, string>& vals)
{
//	if(keys.size()==0)
//		return false;
	string cmd = "mget";
	vals.clear();
	vector<string>::const_iterator it;
	size_t key_size = 0;
	for(it = beg_it; it != end_it; ++it)
	{
		if ((*it).length()==0)
			cmd += (" ThisIsAImpossibleRedisKey");
		else
			cmd += (" "+(*it));
		key_size++;
	}

	redisReply* r = NULL;
    	if(_redis_cnn)
        r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::mget][1] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::mget][2] failed"<<endl;
		return false;
	}
	if(r->type != REDIS_REPLY_ARRAY){
		cerr<<"[MyRedis::mget] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->elements > key_size)
	{
		cerr << "[MyRedis::mget] r->elements > key_size" << endl;
		freeReplyObject(r);
		return false;
	}
	for (size_t i=0;i<r->elements;i++){
		redisReply* childReply = r->element[i];
		if (childReply->type == REDIS_REPLY_STRING){
			vals[*(beg_it+i)]=string(childReply->str,childReply->len);
		}else if (childReply->type == REDIS_REPLY_NIL){
			vals[*(beg_it+i)]="";
		}
#if REDIS_DEBUG
		//fprintf(stderr,"[MyRedis][get][key=%s][val=%s]\n",keys[i].c_str(),vals[keys[i]].c_str());
		std::cerr << "[debug][MyRedis][get][key=" << *(beg_it+i) << "][val=" << vals[*(beg_it+i)] << "]" << std::endl;
#endif
	}

	freeReplyObject(r);

	return true;
}

bool MyRedis::exists(const string& key)
{
	string cmd = "exists " + key;
	bool ret = false;
	redisReply* r = NULL;
    	if(_redis_cnn)
        	r = doCMD(cmd.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd.c_str());
		}else{
			cerr<<"[MyRedis::exists][1] failed"<<endl;
			freeReplyObject(r);
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::exists][2] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->type != REDIS_REPLY_INTEGER){
		cerr<<"[MyRedis::exists] failed"<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->integer == 1)
	{
		ret = true;
	}
	freeReplyObject(r);

	return ret;
}

bool MyRedis::hdel(const std::string& cmd_str)
{
	redisReply* r = doCMD(cmd_str.c_str());
	if (r==NULL){
		if (reConnect()){
			r = doCMD(cmd_str.c_str());
		}else{
			cerr<<"[MyRedis::hdel] failed"<<endl;
			return false;
		}
	}
	if (r==NULL){
		cerr<<"[MyRedis::hdel] failed"<<endl;
		return false;
	}

	if(r->type != REDIS_REPLY_INTEGER) {
		if (r->type != REDIS_REPLY_NIL) {
			freeReplyObject(r);
			return false;
		}
		else {
			freeReplyObject(r);
			return true;
		}

	}

	freeReplyObject(r);

	return true;

}

bool MyRedis::hdel(const std::string& hkey,const std::vector<std::string>& keys)
{
	string cmd = "hdel "+hkey;
	size_t i;
	for (i=0;i<keys.size();i++){
		cmd += (" "+keys[i]);
	}
	if(!hdel(cmd)) return false;
	return true;
}

bool MyRedis::hdel(const std::string& hkey,const tr1::unordered_set<std::string>& keys)
{
	string cmd = "hdel "+hkey;
	for(tr1::unordered_set<std::string>::const_iterator it=keys.begin(); it!=keys.end(); ++it) {
		cmd += (" "+*it);
	}
	if(!hdel(cmd)) return false;
	return true;
}

bool MyRedis::hmset(const std::string& key,const tr1::unordered_map<std::string, std::string>& value_map)
{
	string cmd_args = key;
	vector<const char *> argv;
	vector<size_t> argvlen;
	static char cmd[] = "HMSET";
	argv.push_back( cmd );
	argvlen.push_back( sizeof(cmd)-1 );

	argv.push_back( key.c_str() );
	argvlen.push_back( key.size() );

	for(tr1::unordered_map<string,string>::const_iterator it = value_map.begin(); it != value_map.end(); it++)
	{
		//cmd_args += " "+it->first+" '"+it->second+"'";
	        //cmd_args += " "+it->first+" "+it->second;
		argv.push_back(it->first.c_str());
		argv.push_back(it->second.c_str());
		argvlen.push_back(it->first.size());
		argvlen.push_back(it->second.size());
	}

	redisReply* r = NULL;
	if (_redis_cnn)
	{
//		cerr<<"commond is hmset " << cmd_args.c_str() << endl;
		r = (redisReply*)redisCommandArgv(_redis_cnn, argv.size(),&(argv[0]), &(argvlen[0]));
	}

	if (NULL == r)
	{
		if(reConnect())
		{
		//	r = (redisReply*)redisCommand(_redis_cnn,"hmset %s", cmd_args.c_str());
			r = (redisReply*)redisCommandArgv(_redis_cnn, argv.size(),&(argv[0]), &(argvlen[0]));
		}
		else
		{
			cerr<<"[MyRedis::hmset1] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::hmset2] failed"<<endl;
		return false;
	}

	if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0)){
		cerr<<"[MyRedis::hmset3] failed, info is "<< r->str <<endl;
		freeReplyObject(r);
		return false;
	}

	freeReplyObject(r);
	return true;

}

bool MyRedis::decr(const std::string& key, int& result)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"decr %s",key.c_str());
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"decr %s",key.c_str());
		}
		else
		{
			cerr<<"[MyRedis::decr] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::decr] failed->"<<key<<endl;
		return false;
	}
	if(r->type == REDIS_REPLY_ERROR){
		cerr<<"[MyRedis::decr] failed->"<<key<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->type == REDIS_REPLY_INTEGER)
	{
		result = r->integer;
	}
	else
	{
		cerr<<"[MyRedis::decr] failed, return value is not interger->"<<key<<endl;
		return false;
	}

	freeReplyObject(r);
	return true;
}

bool MyRedis::decrby(const std::string& key, const int decrement, int& result)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"decrby %s %d",key.c_str(),decrement);
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"decrby %s %d",key.c_str(),decrement);
		}
		else
		{
			cerr<<"[MyRedis::decrby] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::decrby] failed->"<<key<<endl;
		return false;
	}
	if(r->type == REDIS_REPLY_ERROR){
		cerr<<"[MyRedis::decrby] failed->"<<key<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->type == REDIS_REPLY_INTEGER)
	{
		result = r->integer;
	}
	else
	{
		cerr<<"[MyRedis::decrby] failed, return value is not interger->"<<key<<endl;
		return false;
	}
	freeReplyObject(r);
	return true;
}

bool MyRedis::incr(const std::string& key, int& result)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"incr %s",key.c_str());
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"incr %s",key.c_str());
		}
		else
		{
			cerr<<"[MyRedis::incr] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::incr] failed->"<<key<<endl;
		return false;
	}
	if(r->type == REDIS_REPLY_ERROR){
		cerr<<"[MyRedis::incr] failed->"<<key<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->type == REDIS_REPLY_INTEGER)
	{
		result = r->integer;
	}
	else
	{
		cerr<<"[MyRedis::incr] failed, return value is not interger->"<<key<<endl;
		return false;
	}

	freeReplyObject(r);
	return true;
}

bool MyRedis::incrby(const std::string& key, const int increment, int& result)
{
	redisReply* r = NULL;
	if (_redis_cnn)
	{
		r = (redisReply*)redisCommand(_redis_cnn,"incrby %s %d",key.c_str(),increment);
	}

	if (NULL == r)
	{
		if(reConnect())
		{
			r = (redisReply*)redisCommand(_redis_cnn,"incrby %s %d",key.c_str(),increment);
		}
		else
		{
			cerr<<"[MyRedis::incrby] failed->[" << key << "]"<<endl;
			return false;
		}
	}

	if (r==NULL){
		cerr<<"[MyRedis::incrby] failed->"<<key<<endl;
		return false;
	}
	
	if(r->type == REDIS_REPLY_ERROR){
		cerr<<"[MyRedis::incrby] failed->"<<key<<endl;
		freeReplyObject(r);
		return false;
	}
	if(r->type == REDIS_REPLY_INTEGER)
	{
		result = r->integer;
	}
	else
	{
		cerr<<"[MyRedis::incrby] failed, return value is not interger->"<<key<<endl;
		return false;
	}

	freeReplyObject(r);
	return true;
}
bool MyRedis::hincrby(const std::string& hkey,const std::string& key,const int increment,int& result)
{
    redisReply* r = NULL;
    if (_redis_cnn) {
        r = (redisReply*)redisCommand(_redis_cnn,"hincrby %s %s %d",hkey.c_str(),key.c_str(),increment);
    }
    if(NULL == r) 
    {
        if(reConnect()) {
            r = (redisReply*)redisCommand(_redis_cnn,"hincrby %s %s %d",hkey.c_str(),key.c_str(),increment);
        }
        else {
		    cerr<<"[MyRedis::hincrby] " << hkey << " | " <<  key << " failed"<<endl;
            return false;
        }
    }
	if(NULL == r)
	{
		cerr<<"[MyRedis::hincrby retry] " << hkey << " | " <<  key << " failed"<<endl;
		return false;
	}

	if(r->type == REDIS_REPLY_ARRAY) {
        result = r->integer;
	    freeReplyObject(r);
	    return true;
    }
	cerr<<"[MyRedis::hincrby] " << hkey << " | " <<  key << " failed"<<endl;
	freeReplyObject(r);
	return false;
}

