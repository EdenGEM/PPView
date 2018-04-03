#include "MySsdb.h"
#include <iostream>
#include <tr1/unordered_map>
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

using namespace std;
using namespace MJ;


//该CTimer只在此头文件中使用，而不对外提供;此计时功能推荐用boost::timer模块代替
class CTimer{
    private:
        struct timeval _last;
        long cost;
    public:
        CTimer(){
            cost=0;

        }
        void reset(){
            cost=0;

        };
        void start(){
            gettimeofday(&_last,NULL);

        };
        void pause(){
            struct timeval _this;
            gettimeofday(&_this,NULL);
            cost+=(_this.tv_sec-_last.tv_sec)*1000000+(_this.tv_usec-_last.tv_usec);

        };
        long get_cost(){
            return cost;

        }

};



MySsdb::MySsdb()
{
    m_ssdbCnn = NULL;
}

MySsdb::~MySsdb()
{
	if (m_ssdbCnn){
		m_ssdbCnn->~Client();
		m_ssdbCnn = NULL;
	}
}

MySsdb::MySsdb(MySsdb* m_ssdb)
{
    m_ssdbCnn = NULL;
    init(m_ssdb->m_ip, m_ssdb->m_port, m_ssdb->m_passwd);
}

bool MySsdb::reConnect()
{
	if(m_ssdbCnn)
	{
		m_ssdbCnn->~Client();
		m_ssdbCnn = NULL;
	}

	m_ssdbCnn = ssdb::Client::connect(m_ip.c_str(), m_port);

	if(NULL == m_ssdbCnn)
	{
		cerr << "[ERROR]:Ssdb Connect retrun NULL]" << endl;;
		return false;
	}

	if(!auth(m_passwd))
	{
		cerr << "[ERROR] ssdb password : "<< m_passwd << " error " << endl;
		return false;
	}
	return true;
}

bool MySsdb::auth(const std::string& passwd)
{
	//std::string cmd = "AUTH "+passwd;
	const std::vector<std::string>* resp_vec = NULL;
	if(m_ssdbCnn)
		resp_vec = m_ssdbCnn->request("auth",passwd);
	else
	{
		if(reConnect())
			resp_vec = m_ssdbCnn->request("auth",passwd);
		else
		{
			cerr << "[ERROR]reConnect error " << endl;
			return false;
		}
	}

	if(resp_vec == NULL)
		return false;

	if((*resp_vec)[0] != "ok")
	{
		cerr << "[ERROR] auth resp != \"OK\", resp size == " << resp_vec->size() << " resp[0] == " << (*resp_vec)[0] << endl;
		return false;
	}
	return true;
}

void MySsdb::warning(long cost, long size, const std::string& type)
{
	if(cost >= max_ssdb_cost_time)
	{
		fprintf(stderr, "[Debug MiojiOPObserver,debuginfo=\"ssdb cmd cost overtime\",threshold=%d,cost=%ld]\n",max_ssdb_cost_time, cost);
		fprintf(stderr, "\n ssdb ip == %s, port == %d\n", m_ip.c_str(), m_port);
		fprintf(stderr, "cost overtime cmd size == %zu, type = %s\n", size, type.c_str());
	}
}

bool MySsdb::init()
{
	return reConnect();
}

bool MySsdb::init(const std::string& ip, const int port, const std::string& passwd)
{
	m_ip = ip;
	m_port = port;
	m_passwd = passwd;

	return reConnect();
}
bool MySsdb::set(const std::string &key, const std::string &val)
{
	CTimer ctimer;
	ctimer.start();

	ssdb::Status s;

	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->set(key, val);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->set(key, val);
		cerr << "[MySsdb::set] reConnect" << endl;
	}

	if(!s.ok())
		cerr << "[MySsdb::set] failed, code == " << s.code() << endl;

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "set");

	return s.ok();
}
bool MySsdb::multi_set(const tr1::unordered_map<std::string, std::string>& kvs)
{
	CTimer ctimer;
	ctimer.start();

	std::map<std::string, std::string> tmp_kvs;
	tmp_kvs.insert(kvs.begin(), kvs.end());
	ssdb::Status s;

	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->multi_set(tmp_kvs);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->multi_set(tmp_kvs);
		cerr << "[MySsdb::set] reConnect" << endl;
	}

	if(!s.ok())
		cerr << "[MySsdb::multi_set] failed, code == " << s.code() << endl;

	ctimer.pause();
	warning(ctimer.get_cost(), tmp_kvs.size(), "multi_set unordered_map");

	return s.ok();
}
bool MySsdb::multi_set(const std::map<std::string, std::string>& kvs)
{
	CTimer ctimer;
	ctimer.start();

	ssdb::Status s;

	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->multi_set(kvs);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->multi_set(kvs);
		cerr << "[MySsdb::multi_set] reConnect" << endl;
	}
	if(!s.ok())
		cerr << "[MySsdb::multi_set] failed, code == " << s.code() << endl;

	ctimer.pause();
	warning(ctimer.get_cost(), kvs.size(), "multi_set std::map");

	return s.ok();
}

bool MySsdb::get(const std::string &key, std::string& val)
{
	CTimer ctimer;
	ctimer.start();

	ssdb::Status s;

	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->get(key, &val);
	}

	//if(!s.ok())
	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->get(key, &val);
		cerr << "[MySsdb::get] reConnect " << endl;
	}

	if(!s.ok())
	{
		cerr << "[MySsdb::get] failed, code == " << s.code() << endl;
	}

	ctimer.pause();
	warning(ctimer.get_cost(), 1,"get");

	return s.ok();
}

bool MySsdb::multi_get(const std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	CTimer ctimer;
	ctimer.start();

	ssdb::Status s;
	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->multi_get(keys, &vals);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->multi_get(keys, &vals);
		cerr << "[MySsdb::Multi_get] reConnect" <<endl;
	}

	if(!s.ok())
		std::cerr << "[ERROR]:Ssdb Multi_get Failed, code == " << s.code() << std::endl;

	ctimer.pause();
	warning(ctimer.get_cost(), keys.size(), "multi_get vector vector");

	return s.ok();
}

bool MySsdb::multi_get(const std::vector<std::string>& keys,std::tr1::unordered_map<std::string,std::string>& kvs)
{
	CTimer ctimer;
	ctimer.start();

	kvs.clear();
	ssdb::Status s;
	std::vector<std::string> vals;
	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->multi_get(keys, &vals);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->multi_get(keys, &vals);
		cerr << "[MySsdb::multi_get] reConnect" << endl;
	}

	if(!s.ok())
		cerr << "[MySsdb::multi_get] failed, code == " << s.code() << endl;

	for(size_t i = 0; i < keys.size(); i ++)
		kvs[keys[i]] = "";
	for(size_t i = 0; i < vals.size(); i += 2)
		kvs[vals[i]] = vals[i+1];

	ctimer.pause();
	warning(ctimer.get_cost(), keys.size(), "multi_get vector unordered_map");

	return s.ok();
}

bool MySsdb::del(const std::string& key)
{
	CTimer ctimer;
	ctimer.start();

	ssdb::Status s;
	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->del(key);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->del(key);
        std::cerr << "[MySsdb::del] reConnect" << std::endl;
	}
    if( !s.ok() )
    {
         cerr << "[MySsdb:del] failed, code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "del");
	return s.ok();
}
bool MySsdb::hget(const std::string& name, const std::string& key, std::string& val)
{
	CTimer ctimer;
	ctimer.start();

	ssdb::Status s;
	if(m_ssdbCnn)
	{
		s = m_ssdbCnn->hget(name, key, &val);
	}

	if(!m_ssdbCnn || s.error())
	{
		if(reConnect())
			s = m_ssdbCnn->hget(name, key, &val);
		else
			std::cerr << "[MySsdb::hget] reConnect " << std::endl;
	}
    if( !s.ok() )
    {
         cerr << "[MySsdb::hget] failed, code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "hget");
	return s.ok();
}

bool MySsdb::hset(const std::string& name, const std::string& key, const std::string& val)
{
	CTimer ctimer;
	ctimer.start();

    ssdb::Status s;
    if(m_ssdbCnn)
    {
		s = m_ssdbCnn->hset(name, key, val);
    }

    if(!m_ssdbCnn || s.error() )
    {
		if(reConnect())
			s = m_ssdbCnn->hset(name, key, val);
		else
			std::cerr << "[MySsdb::hset] reConnect" << std::endl;
    }
    if( !s.ok() )
    {
        cerr << "[MySsdb::hset] failed, code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "hset");
    return s.ok();
}

bool MySsdb::hdel(const std::string& name, const std::string& key)
{
	CTimer ctimer;
	ctimer.start();

    ssdb::Status s;
    if(m_ssdbCnn)
    {
		s = m_ssdbCnn->hdel(name, key);
    }

    if(!m_ssdbCnn || s.error())
    {
		if(reConnect())
			s = m_ssdbCnn->hdel(name, key);
        std::cerr << "[MySsdb::hdel] reConnect" << std::endl;
    }
    if( !s.ok() )
    {
        cerr << "[MySsdb::hdel] failed , code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "hdel");
    return s.ok();
}

bool MySsdb::hsize(const std::string& name, int64_t& ret)
{
	CTimer ctimer;
	ctimer.start();

    ssdb::Status s;
    if(m_ssdbCnn)
    {
		s = m_ssdbCnn->hsize(name, &ret);
    }

    if(!m_ssdbCnn || s.error())
    {
		if(reConnect())
			s = m_ssdbCnn->hsize(name, &ret);
        std::cerr << "[MySsdb::hsize] reConnect " << std::endl;
    }
    if( !s.ok() )
    {
		cerr << "[MySsdb::hsize] failed, code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "hsize");
    return s.ok();
}

bool MySsdb::hkeys(const std::string& name,const std::string& start, const std::string& end, std::vector<std::string>& ret)
{
	CTimer ctimer;
	ctimer.start();

    ssdb::Status s;
	int64_t size = 0;
	hsize(name, size);
    if(m_ssdbCnn)
    {
		s = m_ssdbCnn->hkeys(name, start, end, size, &ret);
    }

    if(!m_ssdbCnn || s.error())
    {
		if(reConnect())
			s = m_ssdbCnn->hkeys(name, start, end, size, &ret);
        std::cerr << "[MySsdb::hkeys] reConnect" << std::endl;
    }
    if( !s.ok() )
    {
		cerr << "[MySsdb::hkeys] failed, code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), 1, "hkeys");
    return s.ok();
}

bool MySsdb::multi_hget(const std::string& name, const std::vector<std::string>& keys,std::vector<std::string>& ret)
{
	std::vector<std::string> resp_vec;
	CTimer ctimer;
	ctimer.start();

    	ssdb::Status s;
    	if(m_ssdbCnn)
    	{
		s = m_ssdbCnn->multi_hget(name, keys, &resp_vec);
    	}

    	if(!m_ssdbCnn || s.error())
    	{
		if(reConnect())
			s = m_ssdbCnn->multi_hget(name, keys, &resp_vec);
        	std::cerr << "[MySsdb::multi_hget] reConnect " << std::endl;
    	}
    	if( !s.ok() )
    	{
		cerr << "[MySsdb::multi_hget] failed, code == " << s.code() << endl;
		return  false;
    	}
	for(size_t i = 1; i < resp_vec.size(); i+=2)
	{
		ret.push_back(resp_vec[i]);
	}

	ctimer.pause();
	warning(ctimer.get_cost(), keys.size(), "hget vector vector");
    	return s.ok();
}

bool MySsdb::multi_hdel(const std::string& name, const std::vector<std::string>& keys)
{
	CTimer ctimer;
	ctimer.start();

    ssdb::Status s;
    if(m_ssdbCnn)
    {
		s = m_ssdbCnn->multi_hdel(name, keys);
    }

    if(!m_ssdbCnn || s.error())
    {
		if(reConnect())
			s = m_ssdbCnn->multi_hdel(name, keys);
        std::cerr << "[MySsdb::multi_hdel] reConnect " << std::endl;
    }

    if( !s.ok() )
    {
		cerr << "[MySsdb::multi_hdel] failed, code == " << s.code() << endl;
    }

	ctimer.pause();
	warning(ctimer.get_cost(), keys.size(), "hget vector vector");
    return s.ok();
}
bool MySsdb::set(const std::string &key, const std::string &val,const time_t& ttl)
{
    ssdb::Status s;
    CTimer ctimer;
    ctimer.start();
    if(m_ssdbCnn)
    {
        s = m_ssdbCnn->setx(key, val,ttl);
    }

    if (!m_ssdbCnn || s.error() )
    {
        if( reConnect() )
            s = m_ssdbCnn->setx(key,val,ttl);
        cerr << "[MySsdb::setx] reConnect" << endl;
    }
    if( !s.ok() )
    {
        cerr << "[MySsdb::setx] failed , code == " << s.code() << endl;
    }
    ctimer.pause();
    warning(ctimer.get_cost(),1,"setx");
    return s.ok();
}

bool MySsdb::hgetall(const std::string& hkey,std::vector<std::string>& keys,std::vector<std::string>& vals)
{
	const std::vector<std::string>* resp_vec = NULL;
	CTimer ctimer;
	ctimer.start();
	if(m_ssdbCnn)
	{
		resp_vec = m_ssdbCnn->request("hgetall", hkey);
    	}
	else
    	{
        	if( reConnect() )
			resp_vec = m_ssdbCnn->request("hgetall", hkey);
		cerr << "[MySsdb::hgetall] reConnect" << endl;
	}
	ssdb::Status s(resp_vec);
	if( !s.ok() )
	{
		cerr << "[MySsdb::hgetall] failed , code == " << s.code() << endl;
		return false;
	}
	for(size_t i = 1; i < resp_vec->size(); i+=2)
	{
		keys.push_back((*resp_vec)[i]);
		vals.push_back((*resp_vec)[i+1]);
	}
	ctimer.pause();
	warning(ctimer.get_cost(),1,"hgetall");
	return s.ok();
}

bool MySsdb::exists(const std::string& key)
{
	bool isExist = false;
	const std::vector<std::string>* resp_vec = NULL;
	CTimer ctimer;
	ctimer.start();
	if(m_ssdbCnn)
	{
		resp_vec = m_ssdbCnn->request("exists", key);
    	}
	else
    	{
        	if( reConnect() )
			resp_vec = m_ssdbCnn->request("exists", key);
		cerr << "[MySsdb::exists] reConnect" << endl;
	}
	ssdb::Status s(resp_vec);
	if( !s.ok() )
	{
		cerr << "[MySsdb::exists] failed , code == " << s.code() << endl;
		return false;
	}
	if(resp_vec->size() > 1)
	{
		if((*resp_vec)[1] == "1")
		{
			isExist = true;
		}
	}
	ctimer.pause();
	warning(ctimer.get_cost(),1,"exists");
	return isExist;
}


bool MySsdb::multi_hset(const std::string& key, const std::map<std::string,std::string>& value_map)
{
	ssdb::Status s;
	CTimer ctimer;
	ctimer.start();
	if(m_ssdbCnn)
    	{
        	s = m_ssdbCnn->multi_hset(key, value_map);
    	}

	if (!m_ssdbCnn || s.error() )
	{
		if( reConnect() )
			s = m_ssdbCnn->multi_hset(key,value_map);
		cerr << "[MySsdb::setx] reConnect" << endl;
	}
	if( !s.ok() )
	{
		cerr << "[MySsdb::setx] failed , code == " << s.code() << endl;
	}
	ctimer.pause();
	warning(ctimer.get_cost(),1,"multi_hset");
	return s.ok();
}

bool MySsdb::expire(const std::string& key, const int ttl)
{
	const std::vector<std::string>* resp_vec = NULL;
	ostringstream oss;
	oss << ttl;
	string ttl_str = oss.str();
	CTimer ctimer;
	ctimer.start();
	if(m_ssdbCnn)
    	{
        	resp_vec = m_ssdbCnn->request("expire", key, ttl_str);
    	}
	else
	{
		if( reConnect() )
			resp_vec = m_ssdbCnn->request("expire", key, ttl_str);
		cerr << "[MySsdb::expire] reConnect" << endl;
	}
	ssdb::Status s(resp_vec);
	if( !s.ok() )
	{
		cerr << "[MySsdb::expire] failed , code == " << s.code() << endl;
	}
	ctimer.pause();
	warning(ctimer.get_cost(),1,"expire");
	return s.ok();
}

bool MySsdb::multi_hset(const std::string& key, const std::map<std::string,std::string>& value_map, const int ttl)
{
	
	CTimer ctimer;
	ctimer.start();
	bool ret = multi_hset(key, value_map);
	if(ret)
	{
		ret = expire(key, ttl);
	}
	ctimer.pause();
	warning(ctimer.get_cost(),1,"multi_hset");
	return ret;
}


/*int main()
{
	MySsdb m_ssdb;
	cout << "begin" << endl;
	string ip = "10.10.198.234";
	if(!m_ssdb.init(ip, 8888))
	{
		cout << "ssdb init err" << endl;
	}
	else
		cout << "ssdb init success" << endl;

	MySsdb m_ssdb2(m_ssdb);
	string b;
	m_ssdb2.get("a",b);
	cout << b << endl;

	return 0;
}*/


