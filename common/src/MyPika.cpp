#include "MyPika.h"
#include <math.h>
#include <unistd.h>

using namespace std;
using namespace MJ;

//MyPika* MyPika::m_pInstance = NULL;

int PikaWorker::doWork(void* ptr)
{
	MyRedis* redis_ptr = (MyRedis*) ptr;
	if(_type == "get")
	{
		if(!redis_ptr->get(_beg_it,_end_it,(*_md5_vals_map_ptr)))
		{
			cerr << "redis get error\n";
			return 0;
		}
//		cerr << "in do work"<< endl;
//		for(size_t i = 0; i < _md5_vals_map_ptr->size(); ++i)
//		{
//			std::tr1::unordered_map<std::string,std::string>::iterator it = _md5_vals_map_ptr->begin();
//			for(;it != _md5_vals_map_ptr->end(); ++it)
//			{
//				cerr <<  it->first << "\t" << it->second << endl;
//			}
//		}
//		cerr << "-------------------------\n";
	}
	return 1;
}

void* PikaThreadPool::bind_ptr()
{
	MyRedis* redis_ptr = new MyRedis;
	if(!redis_ptr->init(m_pika_ipandport, m_pika_pwd))
	{
		cerr << "init pika conn failed\n";
		delete redis_ptr;
		redis_ptr = NULL;
		return NULL;
	}
	pthread_mutex_lock(&m_mutex);
	m_redis_conn_vec.push_back(redis_ptr);
	pthread_mutex_unlock(&m_mutex);
	return (void*)redis_ptr;
}

PikaThreadPool::~PikaThreadPool()
{
	for(size_t i = 0; i < m_redis_conn_vec.size(); ++i)
	{
		delete m_redis_conn_vec[i];
		m_redis_conn_vec[i] = NULL;
	}
}

MyPika::MyPika(MyPika* pika_ptr)
{
	init(pika_ptr->m_pika_ipandport, pika_ptr->m_pika_pwd, pika_ptr->m_thread_num);
}

bool MyPika::init(const string& pika_ipandport, const string& pika_pwd, size_t thread_num, size_t thread_stack_size)
{
	m_thread_num = thread_num;
	pika_thread_pool_ptr = new PikaThreadPool(pika_ipandport, pika_pwd);
	if(pika_thread_pool_ptr->open(thread_num, thread_stack_size) < 0)
	{
		cerr << "init pika thread pool failed\n";
		return false;
	}
	pika_thread_pool_ptr->activate();
	sleep(1);
	return true;
}

bool MyPika::executeCommand(const std::string& type, const std::vector<std::string>& key_vec, vector<std::tr1::unordered_map<string,string> >& result_map_vec)
{
	size_t key_size = key_vec.size();
	int thread_size = (int)ceil((double)key_vec.size()/default_redis_key_size);
	result_map_vec.resize(thread_size);
	size_t count = 0;
	vector<Worker*> worker_vec;
	std::vector<std::string>::const_iterator beg_it;
	std::vector<std::string>::const_iterator end_it;
	for(int i = 0; i < thread_size; i++)
	{
		count += default_redis_key_size;
		beg_it = key_vec.begin() + i*default_redis_key_size;
		if(count < key_size)
		{
			end_it = key_vec.begin() + (i+1)*default_redis_key_size;
		}
		else
		{
			end_it = key_vec.end();
		}
		PikaWorker* pika_ptr = new PikaWorker(type, beg_it, end_it, &result_map_vec[i]);
		worker_vec.push_back(pika_ptr);
		pika_thread_pool_ptr->add_worker(pika_ptr);
	}
	pika_thread_pool_ptr->wait_worker_done(worker_vec);
	/*for(size_t i = 0;  i < result_map_vec.size(); i++)
	{
		cerr << "in executeCommand\n";
		std::tr1::unordered_map<std::string,std::string>::iterator it = result_map_vec[i].begin();
		for(;it != result_map_vec[i].end(); ++it)
		{
			cerr <<  it->first << "\t" << it->second << endl;
		}
		cerr << "============\n";
	}*/
	for(size_t i = 0; i < worker_vec.size(); ++i)
	{
		if (worker_vec[i] != NULL)
		{
			delete worker_vec[i];
			worker_vec[i] = NULL;
		}
	}
	return true;
}

MyPika::~MyPika()
{
	if(pika_thread_pool_ptr != NULL)
	{
		delete pika_thread_pool_ptr;
		pika_thread_pool_ptr = NULL;
	}
}

