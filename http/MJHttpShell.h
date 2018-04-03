#ifndef __MJ_HTTP_SHELL__
#define __MJ_HTTP_SHELL__

#include "http/HttpServer.h"
#include "http/QueryProcessor.h"
#include "http/MJHotUpdate.h"
#include "MJLog.h"
#include <unistd.h>

namespace MJ{

template<class TProcessor, class TUpdate = MJHotUpdate>
class MJHttpShell
{
public:
    MJHttpShell(){
        m_httpServer = NULL;
        m_processor = NULL;
        m_update = NULL;
        m_enableUpdate = -1;
    }
    ~MJHttpShell(){
        m_processor->stop();
        m_httpServer->stop();
        if (m_enableUpdate){
            m_update->stop();
        }
        if (!m_httpServer){
            delete m_httpServer;
            m_httpServer = NULL;
        }

        if (!m_processor){
            delete m_processor;
            m_processor = NULL;
        }
        if (!m_update){
            delete m_update;
            m_update = NULL;
        }
    }

public:
    //初始化方法 必须调用 
    int init(const std::string& module,
            size_t httpThreadNum,
            size_t httpThreadStackSize,
            size_t procThreadNum,
            size_t procThreadStackSize,
            int port,
            const void* config = NULL){
        //设置模块名
        MJHotUpdate::setModule(module);
        //设置热更新共享数据线程数
        MJHotUpdate::setThread(procThreadNum);

        //检查热更新是否先初始化
        if (m_enableUpdate == -1)
            m_enableUpdate = 0;

        int ret;
        
        //配置赋值
        m_httpThreadNum = httpThreadNum;
        m_httpThreadStackSize = httpThreadStackSize;
        m_procThreadNum = procThreadNum;
        m_procThreadStackSize = procThreadStackSize;
        m_port = port;

        //初始化http和processor
        m_httpServer = new HttpServer();
        m_processor = new TProcessor();

        //创建线程
        m_httpServer->link(m_processor);
        m_processor->link(m_httpServer);
        if((ret = m_processor->open(procThreadNum,procThreadStackSize)) < 0){
            MJ_LOG_ERROR("Processor线程创建失败! ret:%d,thread:%zd",ret,procThreadNum);
            return -1;
        }
        if((ret = m_httpServer->open(httpThreadNum,httpThreadStackSize,port)) < 0 ){
            MJ_LOG_ERROR("Http线程创建失败! ret:%d,thread:%zd,listen port:%d",ret,httpThreadNum,port);
            return -2;   
        }

        //最后初始化自定义部分
        if ((ret = m_processor->init(config)) != 0){
            MJ_LOG_ERROR("Processor初始化失败! ret:%d",ret);
            return -3;
        }

        return 0;
    }
    //初始化热更新（如果不需要热更新模块 可以不调用 如果调用 必须在init之前调用）
    int initHotUpdate(const std::vector<MJDBConfig>& configDB,
                        const std::vector<std::string>& monitorList,
                        const int interval,
                        const size_t stackSize){
        //检查热更新是否先初始化
        if (m_enableUpdate != -1){
            MJ_LOG_ERROR("热更新模块初始化失败:必须先初始化MJHotUpdate,然后再初始化http和processor");
            exit(1);
        }
        m_enableUpdate = 1;
        m_update = new TUpdate();

        m_update->init(configDB,monitorList,interval,stackSize);

        return 0;
    }
    //初始化完毕后 调用run启动服务 必须调用
    void run(){
        m_processor->activate();
        m_httpServer->activate();
        if (m_enableUpdate > 0){
            m_update->run();
        }

        sleep(1);

        MJ_LOG("Server初始化完毕,开始运行(http线程%d个,process线程%d个)",m_httpThreadNum,m_procThreadNum);
        MJ_LOG_INFO("[Server started]");
        pause();
    }

    //停止服务 必须调用
    void stop(){
        m_processor->stop();
        m_httpServer->stop();

        m_update->stop();

        MJ_LOG("Server 停止运行");
    }

    //设置服务的任务队列的长度报警阈值（超过则报警，说明有任务堵塞） 可不调用
    void setTaskThreshold(const int threshold){
        m_processor->setTaskThreshold(threshold);
    }



private:
    HttpServer* m_httpServer;
    TProcessor* m_processor;
    TUpdate* m_update;


    int m_enableUpdate;
    int m_httpThreadNum;
    int m_httpThreadStackSize;
    int m_procThreadNum;
    int m_procThreadStackSize;
    int m_port;

    pthread_barrier_t processor_init;
};


}   //namespace MJ







#endif      //__MJ_HTTP_SHELL__
