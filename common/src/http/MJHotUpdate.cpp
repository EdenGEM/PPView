#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <signal.h>



#include "MJHotUpdate.h"
#include "MJLog.h"
#include "AuxTools.h"

using namespace MJ;

pthread_rwlockattr_t MJHotUpdate::s_rwlockAttr;
pthread_rwlock_t MJHotUpdate::s_rwlock;
std::vector<MJSharedData> MJHotUpdate::s_threadData;  //各个process线程对应的数据指针
MJSharedData MJHotUpdate::s_sharedData_cur;   //存放当前正在使用的数据（旧数据）
MJSharedData MJHotUpdate::s_sharedData_new;   //存放新数据
std::string MJHotUpdate::s_module = "";

MJHotUpdateItem::MJHotUpdateItem(){
    database = "";
    table = "";
    lastTS = "";
    newTS = "";
    defer = 0;
    needUpdate = 0;
    dur = new int();
    *dur = 0;
    rowCnt = new int();
    *rowCnt = 0;
}
MJHotUpdateItem::~MJHotUpdateItem(){
    if (dur){
        delete dur;
        dur = NULL;
    }
    if (rowCnt){
        delete rowCnt;
        rowCnt = NULL;
    }
}
void MJHotUpdateItem::dump()const{
    int d = 0;
    int rc = 0;
    if (dur!=NULL) d = *dur;
    if (rowCnt!=NULL) rc = *rowCnt;
    printf("[热更新<%d>][DB:%s#%s][UPDATE:%s->%s][COST:%d][ROW:%d]\n",
        needUpdate, database.c_str(),table.c_str(),lastTS.c_str(),newTS.c_str(),d,rc);
    fflush(stdout);
}

MJHotUpdate::MJHotUpdate(){
    pthread_rwlockattr_init(&MJHotUpdate::s_rwlockAttr);
    pthread_rwlockattr_setkind_np (&MJHotUpdate::s_rwlockAttr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&MJHotUpdate::s_rwlock, &MJHotUpdate::s_rwlockAttr);

    pthread_attr_init(&m_attr);
    pthread_attr_setdetachstate(&m_attr,PTHREAD_CREATE_DETACHED);
}
MJHotUpdate::~MJHotUpdate(){
    std::tr1::unordered_map<std::string,MYSQL*>::iterator it;
    for (it=m_mysqlDB.begin();it!=m_mysqlDB.end();it++){
        mysql_close(it->second);
        delete it->second;
    }

    m_updateItems.clear();

    pthread_rwlock_destroy(&MJHotUpdate::s_rwlock);
    pthread_rwlockattr_destroy(&MJHotUpdate::s_rwlockAttr);

    pthread_attr_destroy(&m_attr);
}

int MJHotUpdate::writeLock(){
    return pthread_rwlock_wrlock(&MJHotUpdate::s_rwlock);
}

int MJHotUpdate::readLock(){
    return pthread_rwlock_rdlock(&MJHotUpdate::s_rwlock);
}

int MJHotUpdate::unlock(){
    return pthread_rwlock_unlock(&MJHotUpdate::s_rwlock);
}

void MJHotUpdate::stop(){
    m_status = 0;
    pthread_kill(m_thread, SIGTERM);
    pthread_join(m_thread, NULL);
}

std::string MJHotUpdate::selectNow(const std::string& db){
    std::string now = "2014-01-01 00:00:00";
    if (m_configDB.find(db) == m_configDB.end()){
        MJ_LOG_ERROR("找不到数据库%s的配置信息",db.c_str());
        exit(1);
    }
    MJDBConfig& conf = m_configDB[db];
    MYSQL mysql;
    mysql_init(&mysql);
    if (!mysql_real_connect(&mysql, conf._addr.c_str(), conf._acc.c_str(), conf._pwd.c_str(), conf._db.c_str(), 0, NULL, 0)){
        MJ_LOG_ERROR("[Connect to %s error]", db.c_str());
        mysql_close(&mysql);
        exit(1);
    }
    if (mysql_set_character_set(&mysql, "utf8")){
        MJ_LOG_ERROR("[Set mysql characterset %s error]",db.c_str());
        mysql_close(&mysql);
        exit(1);
    }
    if (mysql_query(&mysql, "SELECT now()") != 0){
        mysql_close(&mysql);
        return now;
    }
    MYSQL_RES* res = mysql_use_result(&mysql);
    MYSQL_ROW row;
    if(res) {
        row = mysql_fetch_row(res);
        if (row && row[0]!=NULL) {
            now = row[0];
        }
        mysql_free_result(res);
    }
    mysql_close(&mysql);
    return now;
}

void MJHotUpdate::setDefaultUpdateTime(){
    size_t i;
    std::tr1::unordered_map<std::string,MYSQL*>::iterator it;
    for(i=0;i<m_updateItems.size();i++){
        //初始赋值
        m_updateItems[i]->newTS = "2014-01-01 00:00:00";
    }
    return;
}

void MJHotUpdate::getUpdateTime(){
    size_t i;
    MYSQL* mysql;
    std::tr1::unordered_map<std::string,MYSQL*>::iterator it;
    for(i=0;i<m_updateItems.size();i++){
        //初始赋值
        m_updateItems[i]->lastTS = m_updateItems[i]->newTS;
        //赋值mysql
        it = m_mysqlDB.find(m_updateItems[i]->database);
        if (it == m_mysqlDB.end()){
            mysql = new MYSQL();
            mysql_init(mysql);
            MJDBConfig& conf = m_configDB[m_updateItems[i]->database];
            if (!mysql_real_connect(mysql, conf._addr.c_str(), conf._acc.c_str(), conf._pwd.c_str(), conf._db.c_str(), 0, NULL, 0)){
                MJ_LOG_ERROR("[Connect to %s error: %s]", m_updateItems[i]->database.c_str(), mysql_error(mysql));
                delete mysql;
                exit(1);
            }
            if (mysql_set_character_set(mysql, "utf8")){
                MJ_LOG_ERROR("[Set mysql characterset: %s]", mysql_error(mysql));
                delete mysql;
                exit(1);
            }
            m_mysqlDB[m_updateItems[i]->database] = mysql;
        }else
            mysql = it->second;
        //查询更新时间
        std::string sql = "SELECT UPDATE_TIME FROM information_schema.tables WHERE TABLE_SCHEMA='"
                    + m_updateItems[i]->database 
                    + "' and TABLE_NAME='" + m_updateItems[i]->table + "'";
        if(mysql_query(mysql, sql.c_str()) != 0) {
            MJ_LOG_ERROR("查询更新时间失败:%s",sql.c_str());
            continue;
        }else {
            MYSQL_RES* res = mysql_use_result(mysql);
            MYSQL_ROW row;
            if(res) {
                row = mysql_fetch_row(res);
                if (row && row[0]!=NULL)
                    m_updateItems[i]->newTS = row[0];
            }
            mysql_free_result(res);
        }
    }
    return;
}

int MJHotUpdate::init(const std::vector<MJDBConfig>& configDB,
                    const std::vector<std::string>& monitorList,
                    const int interval,
                    const size_t stackSize){
    size_t i;
    //监控时间间隔 单位秒
    m_interval = interval;
    //本机IP
    m_ip = getLocalIP();
    if (m_ip.length()==0){
        MJ_LOG_ERROR("获取IP地址IP失败");
        m_ip = "unknown_ip_addr";
    }
    //初始化配置文件m_configDB
    m_configDB.clear();
    for (i=0;i<configDB.size();i++){
        m_configDB[configDB[i]._db] = configDB[i];
    }
    //热更新状态 0:终止 1:正常运行
    m_status = 1; 
    //初始化监控数据
    m_updateItems.clear();
    for (i=0;i<monitorList.size();i++){
        size_t pos = monitorList[i].find("#");
        if (pos == std::string::npos){
            MJ_LOG_ERROR("热更新初始化参数错误:%s",monitorList[i].c_str());
            return 1;
        }
        std::string db = monitorList[i].substr(0,pos);
        if (m_configDB.find(db) == m_configDB.end()){
            MJ_LOG_ERROR("找不到数据库%s的配置信息",db.c_str());
            exit(1);
        }
        std::string table = monitorList[i].substr(pos+1);
        m_updateItems.push_back(new MJHotUpdateItem());
        m_updateItems[i]->database = db;
        m_updateItems[i]->table = table;
        m_updateItems[i]->lastTS = selectNow(db);
        m_updateItems[i]->newTS = m_updateItems[i]->lastTS;
    }
    //更新更新时间 防止刚启动就重新加载数据
    getUpdateTime();

    //初始化线程栈
    if (pthread_attr_setstacksize(&m_attr,stackSize)){
        MJ_LOG_ERROR("热更新线程栈初始化失败,进程强制退出");
        exit(1);
    }
    
    return 0;
}

//设置服务初始化后的SharedData指针
void MJHotUpdate::initSharedData(void* ptr){
    s_sharedData_cur._data = ptr;
    time_t timestamp;
    time(&timestamp);
    s_sharedData_cur._ts = timestamp;
    MJ_LOG_INFO("热更新Data初始化:%ld",s_sharedData_cur._ts);
    return;
}

//获取对应处理线程的共享数据指针
void* MJHotUpdate::getSharedData(size_t thread_idx){
    return s_threadData[thread_idx]._data;
}
//设置服务名称
void MJHotUpdate::setModule(const std::string& module){
    s_module = module;
}
//初始化共享数据线程数
void MJHotUpdate::setThread(size_t threadCnt){
    s_threadData.clear();
    s_threadData.resize(threadCnt);
}

//添加处理线程的共享数据指针
int MJHotUpdate::addSharedDataPtr(size_t thread_idx){
    MJHotUpdate::readLock();
    if (s_sharedData_new._data != NULL){
        s_threadData[thread_idx] = s_sharedData_new;
    }else{
        s_threadData[thread_idx] = s_sharedData_cur;
    }
    MJHotUpdate::unlock();
    MJ_LOG("QPThread<%ld> Data:%ld",thread_idx,s_threadData[thread_idx]._ts);
    return 0;
}
//删除处理线程的共享数据指针
int MJHotUpdate::delSharedDataPtr(size_t thread_idx){
    MJHotUpdate::readLock();
    s_threadData[thread_idx]._data = NULL;
    s_threadData[thread_idx]._ts = 0;
    MJHotUpdate::unlock();
    return 0;
}

int MJHotUpdate::svc(){
    size_t i;
    struct timeval b,e;
    long timestamp;
    m_status = 1;
    std::string slog = "";
    if (s_sharedData_cur._data == NULL){
        MJ_LOG_ERROR("HotUpdate:没有设置热更新的初始共享内存指针。请先调用MJHotUpdate::setSharedData(void* ptr)");
        exit(1);
    }
    //每隔若干秒轮询
    do{
        sleep(m_interval);
        if (m_updateItems.size()==0)
            continue;

        //0，检测是否需要销毁数据
        if (s_sharedData_new._data != NULL){
            bool canDel = true;
            MJHotUpdate::writeLock();
            for(i=0;i<s_threadData.size();i++){
                if (s_sharedData_cur._data == s_threadData[i]._data){
                    canDel = false;
                    break;
                }
            }
            if (canDel){
                //删除旧数据
                MJ_LOG_INFO("HotUpdate:删除旧数据(%ld)",s_sharedData_cur._ts);
                deleteSharedData(s_sharedData_cur._data);
                s_sharedData_cur = s_sharedData_new;
                s_sharedData_new._ts = 0;
                s_sharedData_new._data = NULL;
            }else{
                MJ_LOG_INFO("HotUpdate:旧数据(%ld)还在使用中,放弃本次热更新",s_sharedData_cur._ts);
            }
            MJHotUpdate::unlock();    
            if (!canDel)
                continue;
        }

        //1, 更新时间
        getUpdateTime();

        //2，解析数据 对比差异
        std::string ts;
        bool needUpdate = false;
        for (i=0;i<m_updateItems.size();i++){
            if (m_updateItems[i]->lastTS == m_updateItems[i]->newTS)
                m_updateItems[i]->needUpdate = 0;
            else{
                m_updateItems[i]->needUpdate = 1;
                needUpdate = true;
                m_updateItems[i]->dump();
            }
        }
        if (!needUpdate)
            continue;

        //3.1，执行热更新,读取更新的数据，重新生成服务所需要的内存数据结构
        int ret = 0;     
        gettimeofday(&b,NULL);
        std::vector<const MJHotUpdateItem*> updateItems;
        updateItems.resize(m_updateItems.size(),NULL);
        for (i=0;i<updateItems.size();i++)
            updateItems[i] = m_updateItems[i];
        void* ptr = this->reloadData(updateItems, s_sharedData_cur._data);
        if (!ptr) {
            struct timeval t;
            gettimeofday(&t,NULL);
            long ts = t.tv_sec*1000+t.tv_usec/1000;
            char qid[32];
            snprintf(qid, 32, "%ld", ts); 
            logMJOB_Exception("ex1003", "", "", qid, ts, m_ip, "", "",MJHotUpdate::s_module);
            ret = 1;
            for (i=0;i<m_updateItems.size();i++){
                m_updateItems[i]->newTS = m_updateItems[i]->lastTS;
            }
            goto WRITE_LOG;
        }

        //3.2，记录下加载数据库相关信息，并还原数据
        
        for (i=0;i<m_updateItems.size();i++){
            if (m_updateItems[i]->needUpdate == 1){
                char buf[256] = {0,};
                snprintf(buf,256,"[%s#%s]:%s->%s num:%d cost:%dms ",
                        m_updateItems[i]->database.c_str(),
                        m_updateItems[i]->table.c_str(),
                        m_updateItems[i]->lastTS.c_str(),
                        m_updateItems[i]->newTS.c_str(),
                        *(m_updateItems[i]->rowCnt),
                        *(m_updateItems[i]->dur));
                slog += buf;
            }
            *(m_updateItems[i]->rowCnt) = 0;
            *(m_updateItems[i]->dur) = 0;
        }

        //3.3 赋值到新指针数据中
        MJHotUpdate::writeLock();
        s_sharedData_new._data = ptr;
        s_sharedData_new._ts = b.tv_sec;
        MJHotUpdate::unlock();
        MJ_LOG_INFO("HotUpdate:生成新数据(%ld)",s_sharedData_new._ts);
        
        //4，输出日志
        WRITE_LOG: 
        gettimeofday(&e,NULL);
        long cost = (e.tv_sec-b.tv_sec)*1000000+(e.tv_usec-b.tv_usec);
        timestamp = e.tv_sec*1000+e.tv_usec/1000;
        logMJOB_Reload(timestamp,m_ip,ret,cost,UrlEncode(slog).c_str());

    }while(m_status==1);

    return 0;
}
void* MJHotUpdate::run_svc(void *arg){
    MJHotUpdate* h = (MJHotUpdate*)arg;
    h->svc();
    return NULL;
}

int MJHotUpdate::run(){
    
    pthread_create(&m_thread, NULL, run_svc, this); 
    return 0;
}
