#ifndef __MY_PIKA_H__
#define __MY_PIKA_H__

#include "threads/MyThreadPool.h"
#include "MyRedis.h"
#include <string>
#include <vector>
#include <tr1/unordered_map>

namespace MJ
{
class PikaWorker:public MJ::Worker
{
	public:
		PikaWorker():_md5_vals_map_ptr(NULL){}
		PikaWorker(const std::string& type, std::vector<std::string>::const_iterator beg_it, std::vector<std::string>::const_iterator end_it, std::tr1::unordered_map<std::string,std::string>* md5_vals_map_ptr)
			:_type(type),_beg_it(beg_it),_end_it(end_it),_md5_vals_map_ptr(md5_vals_map_ptr){}
		virtual int doWork()
		{
			std::cerr << "in wrong do work"<<std::endl;
			return 0;
		}
		virtual int doWork(void* ptr);
		virtual ~PikaWorker(){};
	private:
		std::string _type;
		std::vector<std::string>::const_iterator _beg_it;
		std::vector<std::string>::const_iterator _end_it;
	public:
		std::tr1::unordered_map<std::string,std::string>* _md5_vals_map_ptr;		
};

class PikaThreadPool:public MJ::MyThreadPool
{
	public:
		PikaThreadPool():m_mutex(PTHREAD_MUTEX_INITIALIZER){}
		PikaThreadPool(const std::string& pika_ipandport, const std::string& pika_pwd):m_pika_ipandport(pika_ipandport),m_pika_pwd(pika_pwd),m_mutex(PTHREAD_MUTEX_INITIALIZER)
		{

		}
		void* bind_ptr();
		virtual ~PikaThreadPool();
	private:
		std::string m_pika_ipandport;
		std::string m_pika_pwd;
		std::vector<MJ::MyRedis*> m_redis_conn_vec;
		pthread_mutex_t m_mutex;
};

class MyPika
{
	public:
		MyPika():m_thread_num(0),pika_thread_pool_ptr(NULL)
		{
			m_thread_num = 0;
			pika_thread_pool_ptr = NULL;
		}
		MyPika(MyPika* pika_ptr);
		virtual ~MyPika();
		//{
		//	if(pika_thread_pool_ptr != NULL)
		//	{
		//		delete pika_thread_pool_ptr;
		//		pika_thread_pool_ptr = NULL;
		//	}
		//}
//	public:
//		static MyPika* getInstance()
//		{
//			if(NULL == m_pInstance)
//			{
//				m_pInstance = new MyPika();
//			}
//			return m_pInstance;
//		}
		
//		static void release()
//		{
//			if(NULL != m_pInstance)
//			{
//				delete m_pInstance;
//				m_pInstance = NULL;
//			}
//		}

		bool init(const std::string& pika_ipandport, const std::string& pika_pwd, size_t thread_num = 8, size_t thread_stack_size = default_stack_size);
		bool executeCommand(const std::string& type, const std::vector<std::string>& key_vec, std::vector<std::tr1::unordered_map<std::string,std::string> >& result_map_vec);
	
//	private:
//		static MyPika* m_pInstance;
	
	private:
		size_t m_thread_num;
		PikaThreadPool* pika_thread_pool_ptr;
		std::string m_pika_ipandport;
		std::string m_pika_pwd;
		static const size_t default_redis_key_size = 1000;
		static const size_t default_stack_size = 10*1024*1024;
};
}
#endif
