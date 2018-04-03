#ifndef __MJ_HOT_UPDATE_H__
#define __MJ_HOT_UPDATE_H__

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <pthread.h>
#include <mysql/mysql.h>

namespace MJ{

struct MJDBConfig{
    std::string _addr;      //数据库IP地址
    std::string _acc;      //账户名
    std::string _pwd;       //密码
    std::string _db;        //数据库名
};



class MJSharedData{
public:
    void* _data;     //实体对象指针
    long _ts;    //实体对象的对应数据库时间戳
public:
    MJSharedData(){
        _data = NULL;
        _ts = 0;
    }
};

class MJHotUpdateItem{
public:
    /*不允许热更新update修改的内容*/
    std::string database;   //数据库名称
    std::string table;  //数据库表名
    std::string lastTS; //上一次的更新时间
    std::string newTS;  //最新的更新时间  可以不使用
    int defer;  //数据库延期读取的秒数  停止使用
    int needUpdate; //0:不需要 1:需要更新
    /*以下数据需要在update中修改*/
    int *dur;   //数据库表的读取时间  单位ms
    int *rowCnt;    //数据库表的读取记录数 
public:
    MJHotUpdateItem();
    ~MJHotUpdateItem();
    void dump()const;
};

class MJHotUpdate{
private:
    /*热更新流程相关私有数据*/
    std::tr1::unordered_map<std::string,MJDBConfig> m_configDB;
    std::tr1::unordered_map<std::string,MYSQL*> m_mysqlDB;
    int m_status;   //热更新状态 0:终止 1:正常运行
    std::vector<MJHotUpdateItem*> m_updateItems;    //每个表的热更新相关信息
    int m_interval; //监控时间间隔 单位秒
    std::string m_ip;   //本机IP

    /*线程相关*/
    pthread_t m_thread;
    pthread_attr_t m_attr;

    /*全局静态成员变量-读写锁*/
    static pthread_rwlock_t s_rwlock;
    static pthread_rwlockattr_t s_rwlockAttr;
    
    /*全局静态数据 - 共享数据内存相关数据*/
    static std::vector<MJSharedData> s_threadData;  //各个process线程对应的数据指针
    static MJSharedData s_sharedData_cur;   //存放当前正在使用的数据（旧数据）
    static MJSharedData s_sharedData_new;   //存放新数据
    static std::string s_module;

public:
    MJHotUpdate();
    virtual ~MJHotUpdate();
    /*对外功能函数*/
    //初始化
    virtual int init(const std::vector<MJDBConfig>& configDB,
            const std::vector<std::string>& monitorList,
            const int interval,
            const size_t stackSize=10*1024*1024);
    //主功能函数 轮询热更新
    virtual int run();
    //停止轮询
    virtual void stop();
    //手动设置最后更新时间为2014年 (为了全量更新数据，用在初始化init之后)
    void setDefaultUpdateTime();

    /*对外静态函数*/
    //获取对应处理线程的共享数据指针
    static void* getSharedData(size_t thread_idx);
    //添加处理线程的共享数据指针
    static int addSharedDataPtr(size_t thread_idx);
    //删除处理线程的共享数据指针
    static int delSharedDataPtr(size_t thread_idx);
    //设置服务初始化后的SharedData指针
    static void initSharedData(void* ptr);
    //设置服务名称
    static void setModule(const std::string& module);
    //初始化共享数据线程数
    static void setThread(size_t threadCnt);
    //写操作加锁
    static int writeLock();
    //读操作加锁
    static int readLock();
    //解锁
    static int unlock();

protected:
    //执行热更新,读取更新的数据，重新生成服务所需要的内存数据结构（失败需要销毁新生成的数据）
    //curPtr表示当前内存中的共享数据指针 增量更新需要用到该数据
    virtual void* reloadData(const std::vector<const MJHotUpdateItem*>& items, const void* curPtr)=0;
    //删除旧数据(销毁失败会引起内存泄露)
    virtual int deleteSharedData(void* ptr)=0;
private:
    int svc();
    static void* run_svc(void* arg);
    //获取数据库时间
    std::string selectNow(const std::string& db);
    //更新监控的数据库表的更新时间
    void getUpdateTime();
    
};

}   //namespace MJ

#endif  //__MJ_HOT_UPDATE_H__
