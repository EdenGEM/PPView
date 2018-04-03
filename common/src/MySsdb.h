#ifndef __MY_SSDB_H__
#define __MY_SSDB_H__

#include <iostream>
#include <tr1/unordered_map>
#include <map>
#include <ostream>
#include <sys/time.h>
//SSDB_client.h 是开源头代码,MySsdb.h代替其对外提供接口
#include "ssdb/SSDB_client.h"

namespace MJ
{

const int max_ssdb_cost_time = 800000;
class MySsdb
{
public:
	MySsdb();
	MySsdb(const std::string& ip, const int port, const std::string& passwd="");
	MySsdb(MySsdb* m_ssdb);
	~MySsdb();
	//初始化
	bool init();
	bool init(const std::string& ip, const int port = 8888, const std::string& passwd="");
	bool exists(const std::string& key);
	bool set(const std::string &key, const std::string &val);
	bool get(const std::string &key, std::string& val);
	bool multi_set(const std::tr1::unordered_map<std::string, std::string>& kvs);
	bool multi_set(const std::map<std::string, std::string>& kvs);
	bool multi_get(const std::vector<std::string>& keys,std::tr1::unordered_map<std::string,std::string>& kvs);
    bool multi_get(const std::vector<std::string>& keys, std::vector<std::string>& val);
	bool del(const std::string &key);
	bool auth(const std::string& passwd);
	void warning(long cost, long size, const std::string& type);
	bool hget(const std::string& name, const std::string& key, std::string& val);
	bool hset(const std::string& name, const std::string& key, const std::string& val);
	bool hdel(const std::string& name, const std::string& key);
	bool hsize(const std::string& name, int64_t& ret);
	bool hkeys(const std::string& name,const std::string& start, const std::string& end, std::vector<std::string>& ret);
	bool multi_hget(const std::string& name, const std::vector<std::string>& keys,std::vector<std::string>& ret);
	bool multi_hdel(const std::string& name, const std::vector<std::string>& keys);
    bool set(const std::string &key, const std::string &val,const time_t& ttl);

    	//add by zls
	bool expire(const std::string& key, const int ttl_time);
    	bool hgetall(const std::string& hkey,std::vector<std::string>& keys,std::vector<std::string>& vals);
	bool multi_hset(const std::string& key, const std::map<std::string,std::string>& value_map);
	bool multi_hset(const std::string& key, const std::map<std::string,std::string>& value_map, const int ttl_time);

private:
	bool reConnect();
private:
	ssdb::Client* m_ssdbCnn;
	std::string m_ip;
	int m_port;
	std::string m_passwd;

};

}
#endif
