#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "MJLog.h"
#include <string>
#include "MyTimer.h"
#include "AuxTools.h"
#include "SocketClientEpoll.h"


using namespace MJ;

#define SEG_BUFF_SIZE 256


SocketSession::SocketSession(const std::string& addr,
                int port,
                int timeout,
                MJHttpMethod method,
                const std::string& url_path,
                const std::string& data,
                const std::string& qid,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& ip){
    _addr = addr;
    _port = port;
    _timeout = timeout;
    _method = method;
    _url_path = url_path;
    _data = data;
    _qid = qid;
    _refer_id = refer_id;
    _cur_id = cur_id;
    _ip = ip;
    if (_ip.length()==0)
        _ip = getLocalIP();
    _socket = -1;
    _sendCnt = 0;
    gettimeofday(&_create_time,NULL);

    _response_len = -1;
    _response = "";
}

SocketSession::~SocketSession(){

}



void SocketSession::logRequest(){
    struct timeval t;
    gettimeofday(&t,NULL);
    long ts = t.tv_sec*1000+(t.tv_usec/1000);
    logMJOB_RequestClient("exAPI", "", "",
                            _qid, ts, _ip,
                            _refer_id, _cur_id, "",
                            "", _msg.length());
    return;
}

void SocketSession::logResponse(){
    struct timeval t;
    gettimeofday(&t,NULL);
    long ts = t.tv_sec*1000+(t.tv_usec/1000);
    long cost = ts - (_create_time.tv_sec*1000+(_create_time.tv_usec/1000));
    logMJOB_ResponseClient("exAPI", "", "",
                            _qid, ts, _ip,
                            _refer_id, _cur_id, "",
                            "", _response_len, cost);
    return;
}






MJSocketSession::MJSocketSession(const std::string& addr,
                int port,
                int timeout,
                MJHttpMethod method,
                const std::string& url_path,
                const std::string& type,
                const std::string& qid,
                const std::string& query,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& ptid,
                const std::string& lang,
                const std::string& ccy,
                const std::string& csuid,
                const std::string& uid,
                const std::string& auth){
    _addr = addr;
    _port = port;
    _timeout = timeout;
    _method = method;
    _url_path = url_path;
    _type = type;
    _qid = qid;
    _query = query;
    _ip = ip;
    if (_ip.length()==0)
        _ip = getLocalIP();
    _refer_id = refer_id;
    _cur_id = cur_id;
    _ptid = ptid;
    _auth = auth;
    _lang = lang;
    _ccy = ccy;
    _csuid = csuid;
    _uid = uid;

    md5(_type+_query,_next_id);

    _socket = -1;
    _sendCnt = 0;
    _msg = "";
    gettimeofday(&_create_time,NULL);

    _response_len = -1;
    _response = "";
}

MJSocketSession::~MJSocketSession(){

}



void SocketClientEpoll::setNonBlock(int fd) {   
    int flag = fcntl ( fd, F_GETFL, 0 );   
    fcntl ( fd, F_SETFL, flag | O_NONBLOCK );   
} 

void closeSession(SocketSession* session, int epoll_fd, int& restSession, int stat){
    //断开连接
    if (session->_socket > 0){
        epoll_ctl(epoll_fd,EPOLL_CTL_DEL,session->_socket,NULL);
        close(session->_socket); 
    }

    if (session->_response_len == -1){
        restSession--;
    }

    session->_msg = "";

    if (stat == -1){
        //请求正常返回
        session->_response_len = session->_response.length();
        //截取报文内容
        size_t pos = session->_response.find("\r\n\r\n");
        size_t pos_gzip = session->_response.rfind("Content-Encoding: gzip\r\n",pos);
        if (pos!=std::string::npos){
            //解压缩
            if (pos_gzip != std::string::npos){
                MJ::decompress_gzip(session->_response.substr(pos+4),session->_response);
            }else{
                session->_response = session->_response.substr(pos+4);
            }
        }
    }else{
        //请求异常
        session->_response_len = stat;
    }

    return;
}

void MJSocketSession::logRequest(){
    struct timeval t;
    gettimeofday(&t,NULL);
    long ts = t.tv_sec*1000+(t.tv_usec/1000);
    logMJOB_RequestClient(_type, _uid, _csuid,
                            _qid, ts, _ip,
                            _refer_id, _cur_id, _next_id,
                            "", _msg.length());
    return;
}

void MJSocketSession::logResponse(){
    struct timeval t;
    gettimeofday(&t,NULL);
    long ts = t.tv_sec*1000+(t.tv_usec/1000);
    long cost = ts - (_create_time.tv_sec*1000+(_create_time.tv_usec/1000));
    logMJOB_ResponseClient(_type, _uid, _csuid,
                            _qid, ts, _ip,
                            _refer_id, _cur_id, _next_id,
                            "", _response_len, cost);
    return;
}
void addEpollOut(int efd, SocketSession* session){
    //向epoll实例绑定要监控的fd及监控事件
    struct epoll_event event; 
    event.events = EPOLLOUT | EPOLLIN | EPOLLET;
    event.data.ptr = session;
    epoll_ctl(efd, EPOLL_CTL_ADD, session->_socket, &event); 
    return;
}
void modEpollIn(int efd, SocketSession* session){
    struct epoll_event event; 
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = session;
    epoll_ctl(efd,EPOLL_CTL_MOD,session->_socket,&event);
    return;
}
void delEpoll(int efd, SocketSession* session){
    epoll_ctl(efd, EPOLL_CTL_DEL, session->_socket, NULL);
    return;
}

void MJSocketSession::makeHttpRequest(){
    std::string url;
    makeGetUrl(url);
    const char* qr = url.c_str();
    if (*qr == '/')
        qr++;
    /*生成请求完整报文*/
    _msg = (std::string)"GET /" + qr + " HTTP/1.0\r\n"+
            (std::string)"Accept: */*\r\n" +
            (std::string)"Accept-Language: zh-cn\r\n" +
            (std::string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n";
    return;
}

void MJSocketSession::makeGetUrl(std::string& url){
    /*对GET请求的URL的参数部分进行URLEncode*/
    url = _url_path + "?" + "type=" + UrlEncode(_type) + 
                            "&lang=" + UrlEncode(_lang) +
                            "&qid=" + UrlEncode(_qid) +
                            "&ptid=" + UrlEncode(_ptid) +
                            "&auth=" + UrlEncode(_auth) +
                            "&ccy=" + UrlEncode(_ccy) +
                            "&query=" + UrlEncode(_query) + 
                            "&refer_id=" + UrlEncode(_refer_id) +
                            "&cur_id=" + UrlEncode(_cur_id) +
                            "&next_id=" + UrlEncode(_next_id) +
                            "&uid=" + UrlEncode(_uid) +
                            "&csuid=" + UrlEncode(_csuid);
    
    return;
}


int SocketClientEpoll::doHttpRequest(SocketSession* session){
    std::vector<SocketSession*> tasks;
    tasks.push_back(session);
    return doHttpRequest(tasks,session->_timeout);
}

int SocketClientEpoll::doHttpRequest(const std::vector<SocketSession*>& sessions,int timeout){
    /*通用: 创建EPOLL监听*/
    int efd;
    size_t i;    
    int restSession = sessions.size();
    efd = epoll_create(10);    
    if(efd == -1) {   
        MJ_LOG_ERROR("epoll_create失败");
        return -1;
    }
    size_t eventsLen = restSession*5;
    struct epoll_event events[eventsLen];

    /*生成请求报文*/
    SocketSession* session;
    std::string requestParam = "";
    std::string requestStr = "";
    for (i=0;i<sessions.size();i++){
        session = sessions[i];
        /*对GET请求的URL的参数部分进行URLEncode*/
        std::string requestStr;
        session->makeHttpRequest();
        session->logRequest();
    }
    requestParam = "";
    requestStr = "";

    for (i=0;i<sessions.size();i++){
        session = sessions[i];
        /*为请求创建socket*/
        int sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
        if(sk < 0) {   
            MJ_LOG_ERROR("[sock:%d]创建失败",sk);
            closeSession(session,efd,restSession,-4);
            //添加响应日志
            session->logResponse();
            continue;
        }
        session->_socket = sk;
        struct sockaddr_in sa = {0};   
        sa.sin_family = AF_INET;   
        sa.sin_port = htons(session->_port);   
        struct sockaddr_in *psa = &sa;   
        int iRet = inet_pton(AF_INET, session->_addr.c_str(), &psa->sin_addr.s_addr); 
        if (iRet <= 0){
            MJ_LOG_ERROR("[sock:%d]无效URL地址:[%s]",sk,session->_addr.c_str());
            closeSession(session,efd,restSession,-5);  
            //添加响应日志
            session->logResponse();
            continue;
        }
        //设置非阻塞
        SocketClientEpoll::setNonBlock(sk);
        //连接
        connect(sk, (struct sockaddr*)&sa, sizeof(sa));
        //注册监听事件
        addEpollOut(efd,session);
    }

    char buf[READ_BUF_SIZE_CLIENT + 1];

    while(restSession > 0){
        MyTimer tmr;
        int n = epoll_wait(efd, events, eventsLen, timeout); 
        if (n == 0){
            MJ_LOG_ERROR("请求超时\n");
            goto ALL_OVER;
        }  
        for(int i = 0; i < n; i++){  
            session = (MJSocketSession*) events[i].data.ptr;
            if(events[i].events & EPOLLOUT){ 
                int writeCnt;
                while(true){ 
                    const char* rest_buff = session->_msg.c_str()+session->_sendCnt; 
                    int len = session->_msg.length() - session->_sendCnt;
                    if (len <= 0){
                        //发送完毕
                        modEpollIn(efd,session);
                        break;
                    } 
                    writeCnt = write(session->_socket,rest_buff,len);
                    // printf("write: %d\n", writeCnt);
                    if (writeCnt > 0){
                        if (writeCnt >= len){
                            //发送完毕
                            // printf("发送完毕\n");
                            modEpollIn(efd,session);
                            break;
                        }
                        //继续发送
                        session->_sendCnt += writeCnt;
                        continue;
                    }else if (writeCnt == 0){
                        MJ_LOG_ERROR("socket(%d)Send被服务端关闭",session->_socket);
                        closeSession(session,efd,restSession,-2);
                        //添加日志
                        session->logResponse();
                        break;
                    }else{
                        if (errno == EAGAIN || errno == EWOULDBLOCK){
                            //缓冲区满 等待下次发送
                            // MJ_LOG_ERROR("write未结束");
                            break;
                        }else{
                            MJ_LOG_ERROR("Send失败(errno=%d),关闭连接",errno);
                            closeSession(session,efd,restSession,0);
                            //添加日志
                            session->logResponse();
                            break;
                        }
                    }   
                } 
            }else if(events[i].events & EPOLLIN) {   
                int readCnt;
                while(true){
                    readCnt = read(session->_socket, buf, READ_BUF_SIZE_CLIENT);
                    // printf("read: %d\n", readCnt);
                    if (readCnt > 0){
                        buf[readCnt] = '\0';
                        // session->_response += buf;
                        session->_response.append(buf,readCnt);
                        continue;
                    }else if (readCnt == 0){
                        // MJ_LOG_ERROR("socket(%d)Read完成",session->_socket);
                        closeSession(session,efd,restSession,-1);
                        //添加日志
                        session->logResponse();
                        break;
                    }else{
                        // printf("errno:%d\n", errno);
                        if (errno == EAGAIN || errno == EWOULDBLOCK){
                            //重置监控事件，等待再次接收
                            // MJ_LOG_ERROR("read未结束");
                            modEpollIn(efd,session);
                            break;
                        }else{
                            MJ_LOG_ERROR("Read失败(errno=%d),关闭连接",errno);
                            closeSession(session,efd,restSession,0);
                            //添加日志
                            session->logResponse();
                            break;
                        }
                    }
                }       
            }else{
                MJ_LOG_ERROR("Epoll错误事件(%d),关闭连接",events[i].events);
                closeSession(session,efd,restSession,0);
                //添加日志
                session->logResponse();
            }  
        } 
        //不限超时时间的话 直接继续循环
        if (timeout < 0)
            continue;
        timeout -= int(tmr.cost()/1000); 
        if (timeout < 0)
            timeout = 0;
    }

    ALL_OVER:
    for(i=0;i<sessions.size();i++){
        if (sessions[i]->_response_len == -1){
            closeSession(sessions[i],efd,restSession,-3);
            //添加响应日志
            sessions[i]->logResponse();
        }
    }
    close(efd);

    return 0;
}


void SocketSession::makeHttpRequest(){
    const char* qr = _url_path.c_str();    
    if (*qr == '/')
        qr++;
    if (_method == Get){
        _msg = (std::string)"GET /" + qr + "?" + _data + " HTTP/1.0\r\n"+
            (std::string)"Accept: */*\r\n" +
            (std::string)"Accept-Language: zh-cn\r\n" +
            (std::string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n";
    }else{
        _msg = (std::string)"POST /" + qr + " HTTP/1.0\r\n"+
            (std::string)"Accept: */*\r\n" +
            (std::string)"Accept-Language: zh-cn\r\n" +
            (std::string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n"
            + _data;
    }
    
    return;
}
