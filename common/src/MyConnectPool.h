#ifndef __MJ_CONNECT_POOL_H__
#define __MJ_CONNECT_POOL_H__

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>

namespace MJ
{

template<class T>
class MJConnectPool{
private:
    pthread_mutex_t *m_mutex;
    //所有连接当前状态 0:未使用 1:使用中
    std::vector<unsigned char> m_stats;
    //所有连接
    std::vector<T*> m_cnns;
    //连接总数量
    size_t m_cnt;
    //下一个检索开始的连接索引
    size_t m_index;
public:
    int init(const std::vector<T*>& connects){
        if(connects.empty())
            return 1;
        pthread_mutex_lock(m_mutex);
        m_cnt = connects.size();
        m_index = 0;
        for(size_t i = 0; i < connects.size(); i++){
            m_stats.push_back(0);
            m_cnns.push_back(connects[i]);
        }
        pthread_mutex_unlock(m_mutex);
        return 0;
    }

    size_t alloc(T*& cnn){
        cnn = NULL;
        size_t index = 0;
        do{
            pthread_mutex_lock(m_mutex);
            for (size_t i=m_index; i<m_cnt + m_index; i++){
                index = i%m_cnt;
                if (m_stats[index] == 0){
                    cnn = m_cnns[index];
                    m_stats[index] = 1;
                    m_index = (index + 1)%m_cnt;
                    break;
                }
            }
            pthread_mutex_unlock(m_mutex);
            if (cnn == NULL){
            //    std::cerr<<"[Warning]:MJConnectPool连接数不够用\n";
                usleep(3);
            }
        }while(cnn == NULL);
        return index;
    }

    bool release(const size_t& index){
        if(index < m_cnt){
            m_stats[index] = 0;
            return true;
        }
        return false;
    }
    MJConnectPool(){
        m_mutex = new pthread_mutex_t;
        pthread_mutex_init(m_mutex, NULL);
    }

    ~MJConnectPool(){
        if(NULL != m_mutex) {
            delete m_mutex;
            m_mutex = NULL;
        }
    }
};

}

#endif  //__MJ_CONNECT_POOL_H__
