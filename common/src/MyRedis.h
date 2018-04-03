#ifndef __MY_REDIS_H__
#define __MY_REDIS_H__

#include <vector>
#include <string>
#include "hiredis/hiredis.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <map>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

namespace MJ
{
const int max_redis_cost_time = 800000;

struct StringItem
{
	std::string key;
	std::string val;
	const char* pkey[5];
};

class MyRedis{
public:
	MyRedis();
    MyRedis(MyRedis* m_redis);
	~MyRedis();
	//初始化
	bool init(const std::string& addr,const std::string& password,const int db = 0);
	/*执行命令*/
	bool get(const std::string& key,std::string& val);		//获取指定key的val
	bool get(const std::vector<std::string>& keys,std::vector<std::string>& vals);		//获取多个key的val
	bool get(const std::vector<std::string>& keys,std::tr1::unordered_map<std::string,std::string>& vals);
	bool get(std::vector<std::string>::const_iterator beg_it,std::vector<std::string>::const_iterator end_it,
			std::tr1::unordered_map<std::string,std::string>& vals);
	bool hget(const std::string& hkey,const std::vector<std::string>& keys,std::vector<std::string>& vals);		//获取多个key的val
	bool hgetall(const std::string& hkey,std::vector<std::string>& keys,std::vector<std::string>& vals);
	bool set(const std::string& key,const std::string& val);
	bool set_old(const std::string& key,const std::string& val);
	bool keys(const std::string& key, std::vector<std::string>& valkeys);
	bool del(const std::string&);
	bool set_expire(const std::string& key,size_t expire_seconds);
	bool set_value_and_expire(const std::string& key, const std::string& val,const size_t& ttl_time);
    bool ttl(const std::string &key, int *ttl_time);
	bool mset(const std::string& key_value);

	//-1 error;0 exist;1 set success
	int setnx(const std::string& key,const std::string& val);
	int setnx_and_expire(const std::string& key, const std::string& val,const size_t& ttl_time);

	bool scan(const unsigned long& cursor, const int&count, unsigned long& next_cursor, std::vector<std::string>& keys_vec);
	bool hmset(const std::string& key, const std::map<std::string,std::string>& value_map);
	bool mset(const std::map<std::string,std::string>& key_value_map);
	bool hmset_value_and_expire(const std::string& key, const std::map<std::string,std::string>& value_map, const size_t& ttl_time);
    bool isConnected();
	bool scan(const unsigned long& cursor, unsigned long& next_cursor, std::vector<std::string> keys_vec);
	bool smembers(const std::string& key, std::vector<std::string>& vals);

	bool sinter(const std::vector<std::string>& keys,std::vector<std::string>& vals);
	bool sunion(const std::vector<std::string>& keys,std::vector<std::string>& vals);

	//批量的将元素加入到set集合中
	bool sadd(const std::string& key,const std::tr1::unordered_set<std::string>& value_set);
	bool sadd(const std::string& key,const std::string& value_str_sep_by_space);
	
	//批量的移除set集合中的元素
	bool srem(const std::string& key,const std::tr1::unordered_set<std::string>& value_set);
	bool srem(const std::string& key,const std::string& value_str_sep_by_space);

	//获取集合中的元素的数量
	bool scard(const std::string& key,int& val_count);
	//将值value关联到key，并设置key的生存时间
	bool setex(const std::string& key,const std::string& val,const int expire_seconds);
	//获取集合中的所有元素
	bool smembers(const std::string& key,std::tr1::unordered_set<std::string>& val_set);

	bool exists(const std::string& key);

	bool hdel(const std::string& cmd_str);
	bool hdel(const std::string& hkey,const std::vector<std::string>& keys);
	bool hdel(const std::string& hkey,const std::tr1::unordered_set<std::string>& keys);

	bool hmset(const std::string& key,const std::tr1::unordered_map<std::string, std::string>& value_map);
	
	bool decr(const std::string& key, int& result);
	bool decrby(const std::string& key, const int decrement, int& result);

	bool incr(const std::string& key, int& result);
	bool incrby(const std::string& key, const int increment, int& result);
    bool hincrby(const std::string& hkey,const std::string& key,const int increment,int& result);

	bool flushDb();

	std::string _addr;
	std::string _password;
private:
	redisReply* doCMD(const char* key);
	redisContext* select_db(redisContext *c);
	bool reConnect();
private:
	redisContext* _redis_cnn;
	std::string _ip;
	int _port;
	int _db;
	pthread_mutex_t mutex_locker_;
};

}
#endif	//__MY_REDIS_H__
