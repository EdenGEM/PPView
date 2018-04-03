#include <string>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <tr1/unordered_map>

#include "http/TaskBase.h"
#include "AuxTools.h"
#include "MJLog.h"
#include "MyTimer.h"



using namespace std;
using namespace MJ;


string g_addr = "127.0.0.1";
int g_port = -1;
double g_qps = 1.0;
int g_duration = 1;
int g_debug = 0;
size_t g_thread_num = 1;





char g_buf[1024*1024];


#define READ_BUF_SIZE_CLIENT (128*1024)

class MyHttpRequest{
public:
    /*输入参数*/
    std::string _addr;
    int _port;
    int _timeout;  //超时时间 单位毫秒
    std::string _url_path;  //请求的url path  后台请求一般为空串
    long _qid;       //客户端最初的请求ID 表示一个业务 客户端使用时间戳即可（精确到毫秒)
    std::string _refer_id;  //上游传来的请求ID
    std::string _cur_id;    //本服务的请求ID
    std::string _ip;        //打请求的服务IP
    std::string _type;      //请求类型
    std::string _lang;      //语言版本 "en|zh_cn"
    std::string _ptid;      //合作方ID
    std::string _auth;      //合作方权限
    std::string _ccy;       //当前期望的货币类型 CNY HKD USD EUR  默认为CNY
    std::string _csuid;        //客服ID
    std::string _uid;        //终端用户ID
    std::string _query;     //json对象的字符串，每个接口的业务逻辑都在这里

    /*中间变量*/
    int _socket;
    int _sendCnt;
    std::string _msg;       //请求报文
    std::string _next_id;      //下游请求ID 根据请求生成
    long _recv_time;        //返回结果的时间
    int _idx;       //请求索引


    /*输出参数*/
    int _response_len;  //-1:表示请求未被处理完 0:请求失败(未知原因) >0:请求成功(含报头长度) -2:请求被断开 -3:请求超时 -4:socket创建失败 -5:无效IP
    std::string _response;  //实际收到的回报的报文 （不含报头）
public:
    MyHttpRequest():_addr(""),_port(-1),_url_path(""),_socket(-1),_sendCnt(0),_msg(""),_next_id(""),_recv_time(0),_idx(-1),_response_len(0),_response(""){
        _timeout = -1;
        _qid = 0;
        _refer_id = "";
        _cur_id = "";
        _ip = "";
        _type = "";
        _lang = "";
        _ptid = "";
        _auth = "";
        _ccy = "CNY";
        _csuid = "";
        _uid = "";
        _query = "";
    }
};


MyHttpRequest parseParams(const std::string& data_str){
    MyHttpRequest ret;
    ret._addr = g_addr;
    ret._port = g_port;

    size_t pos = 0;
    std::string key;
    std::string val;
    size_t len = data_str.length();
    std::tr1::unordered_map<std::string,std::string> data;
    while(pos < len){
        size_t t = data_str.find("=",pos);
        if (t == std::string::npos)
            break;
        key = data_str.substr(pos,t-pos);
        pos = t+1;
        t = data_str.find("&",pos);
        if (t == std::string::npos){
            val = UrlDecode(data_str.substr(pos));
            data[key] = val;
            break;
        }else{
            val = UrlDecode(data_str.substr(pos,t-pos));
            pos = t+1;
            data[key] = val;
        }
    }
    

    std::tr1::unordered_map<std::string,std::string>::iterator it;
    it = data.find("type");
    if (it != data.end()){
        ret._type = it->second;
    }

    it = data.find("lang");
    if (it != data.end()){
        ret._lang = it->second;
    }

    // it = data.find("qid");
    // if (it != data.end()){
    //     ret._qid = it->second;
    // }

    it = data.find("ptid");
    if (it != data.end()){
        ret._ptid = it->second;
    }

    it = data.find("auth");
    if (it != data.end()){
        ret._auth = it->second;
    }

    it = data.find("ccy");
    if (it != data.end()){
        ret._ccy = it->second;
    }

    it = data.find("csuid");
    if (it != data.end()){
        ret._csuid = it->second;
    }

    it = data.find("uid");
    if (it != data.end()){
        ret._uid = it->second;
    }

    it = data.find("query");
    if (it != data.end()){
        ret._query = it->second;
    }


    it = data.find("cur_id");
    if (it != data.end()){
        ret._refer_id = it->second;
    }

    it = data.find("next_id");
    if (it != data.end()){
        ret._cur_id = it->second;
    }
    return ret;
}

void loadCases(const string& file, vector<MyHttpRequest>& cases){
    ifstream fin(file);
    if (!fin){
        cerr<<"file open failed! "<<file<<endl;
        return;
    }
    string line = "";
    while(!fin.eof()){
        getline(fin,line);
        if (line.length() == 0 || line[0] == '#')
            continue;
        cases.push_back(parseParams(line));
    }
    return;
}


class MJLoader : public TaskBase{
public:
    int m_cnt;  //请求总数
    int m_recvCnt;  //返回的请求总数
    long m_recvLen;  //返回的请求总长度
    int m_failCnt; //失败的请求总数
    int m_exCnt;    //返回的请求中，异常的请求总数(error_id != 0)
    long m_dur;     //所有返回请求（m_recvCnt）的总时间  单位us 微妙
    int m_stop;
    vector<int> m_efds;  //epoll
    vector<int> m_recvCnts;
    vector<long> m_recvLens;
    vector<int> m_failCnts; //失败的请求总数
    vector<int> m_exCnts;    //返回的请求中，异常的请求总数(error_id != 0)
    vector<long> m_durs;     //所有返回请求（m_recvCnt）的总时间  单位us 微妙
public:
    MJLoader():TaskBase(){
        m_stop = 0;
        m_cnt = 0;
        m_recvLen = 0;
        m_recvCnt = 0;
        m_failCnt = 0;
        m_exCnt = 0;
        m_dur = 0;
        m_efds.clear();
    }
    void dump(){
        m_recvCnt = 0;
        for (size_t i=0;i<m_recvCnts.size();i++)
            m_recvCnt += m_recvCnts[i];

        m_failCnt = 0;
        for (size_t i=0;i<m_failCnts.size();i++)
            m_failCnt += m_failCnts[i];

        m_recvLen = 0;
        for (size_t i=0;i<m_recvLens.size();i++)
            m_recvLen += m_recvLens[i];

        m_exCnt = 0;
        for (size_t i=0;i<m_exCnts.size();i++)
            m_exCnt += m_exCnts[i];

        m_dur = 0;
        for (size_t i=0;i<m_durs.size();i++)
            m_dur += m_durs[i];


        fprintf(stderr, "########测试结果###########\n");
        fprintf(stderr, "请求总数: %d\n", m_cnt);
        fprintf(stderr, "请求失败: %d, 占%%%.2f\n", m_failCnt, (double)m_failCnt*100.0/m_cnt);
        fprintf(stderr, "请求丢失: %d, 占%%%.2f\n", m_cnt - m_recvCnt - m_failCnt, (double)(m_cnt - m_recvCnt - m_failCnt)*100.0/m_cnt);
        fprintf(stderr, "请求异常(error_id不为0): %d, 占%%%.2f\n", m_exCnt, (double)m_exCnt*100.0/m_cnt);
        fprintf(stderr, "请求成功: %d, 占%%%.2f\n", m_recvCnt - m_exCnt, (double)(m_recvCnt - m_exCnt)*100.0/m_cnt);
        fprintf(stderr, "平均返回长度(包括异常): %ld\n", m_recvLen/m_recvCnt);
        fprintf(stderr, "平均响应时间(包括异常): %ldus\n", m_dur/m_recvCnt);
        fprintf(stderr, "########  END  ###########\n");
    }
    bool isOver(){
        int m_over = 0;
        for (size_t i=0;i<m_recvCnts.size();i++)
            m_over += m_recvCnts[i];
        for (size_t i=0;i<m_failCnts.size();i++)
            m_over += m_failCnts[i];
        return m_cnt == m_over;
    }
    virtual int stop(){
        m_stop = 1;
        join();
        return 0;
    }
    virtual int open(size_t thread_num, size_t stack_size){
        m_efds.resize(thread_num,-1);
        m_recvCnts.resize(thread_num,0);
        m_recvLens.resize(thread_num,0);
        m_failCnts.resize(thread_num,0);
        m_exCnts.resize(thread_num,0);
        m_durs.resize(thread_num,0);
        return TaskBase::open(thread_num, stack_size);
    }
    virtual int svc(size_t idx){
        /*通用: 创建EPOLL监听*/
        int m_efd = epoll_create(10);
        m_efds[idx] = m_efd;
        if(m_efd == -1) {   
            MJ_LOG_ERROR("epoll_create失败");
            exit(1);
        }
        size_t eventsLen = 256;
        struct epoll_event events[eventsLen];
        char buf[READ_BUF_SIZE_CLIENT + 1];

        MJ_LOG_INFO("Socket处理线程<%zd>启动成功", idx);

        while(!m_stop){
            int n = epoll_wait(m_efd, events, eventsLen, -1); 
            for(int i = 0; i < n; i++){  
                MyHttpRequest* session = (MyHttpRequest*) events[i].data.ptr;
                if(events[i].events & EPOLLOUT){ 
                    int writeCnt;
                    while(true){ 
                        const char* rest_buff = session->_msg.c_str()+session->_sendCnt; 
                        int len = session->_msg.length() - session->_sendCnt;
                        if (len <= 0){
                            modEpollIn(m_efd,session);
                            break;
                        } 
                        writeCnt = write(session->_socket,rest_buff,len);
                        if (writeCnt > 0){
                            if (writeCnt >= len){
                                modEpollIn(m_efd,session);
                                break;
                            }
                            session->_sendCnt += writeCnt;
                            continue;
                        }else if (writeCnt == 0){
                            MJ_LOG_ERROR("socket(%d)Send被服务端关闭",session->_socket);
                            closeSession(session,m_efd,-2);
                            m_failCnts[idx]++;
                            break;
                        }else{
                            if (errno == EAGAIN || errno == EWOULDBLOCK){
                                break;
                            }else{
                                MJ_LOG_ERROR("Send失败(errno=%d),关闭连接",errno);
                                closeSession(session,m_efd,0);
                                m_failCnts[idx]++;
                                break;
                            }
                        }   
                    } 
                }else if(events[i].events & EPOLLIN) {   
                    int readCnt;
                    while(true){
                        readCnt = read(session->_socket, buf, READ_BUF_SIZE_CLIENT);
                        if (readCnt > 0){
                            buf[readCnt] = '\0';
                            session->_response.append(buf,readCnt);
                            continue;
                        }else if (readCnt == 0){
                            closeSession(session,m_efd,-1);
                            m_recvCnts[idx]++;
                            m_recvLens[idx] += session->_response_len;
                            struct timeval t;
                            gettimeofday(&t,NULL);
                            session->_recv_time = t.tv_sec*1000000+t.tv_usec;
                            m_durs[idx] += (session->_recv_time - session->_qid);
                            if (session->_response.find("\"error_id\":0") == std::string::npos)
                                m_exCnts[idx]++;
                            if (g_debug)
                                fprintf(stderr, "RESP<%d> qid:%ld cost:%ld, len:%d\n", session->_idx, session->_qid, session->_recv_time - session->_qid, session->_response_len);
                            delete session;
                            break;
                        }else{
                            if (errno == EAGAIN || errno == EWOULDBLOCK){
                                modEpollIn(m_efd,session);
                                break;
                            }else{
                                MJ_LOG_ERROR("Read失败(errno=%d),关闭连接",errno);
                                closeSession(session,m_efd,0);
                                m_failCnts[idx]++;
                                break;
                            }
                        }
                    }       
                }else{
                    MJ_LOG_ERROR("Epoll错误事件(%d),关闭连接",events[i].events);
                    closeSession(session,m_efd,0);
                    m_failCnts[idx]++;
                }  
            } 
        }
        return 0;
    }
    void request(const MyHttpRequest& req){
        m_cnt++;

        MyHttpRequest* s = new MyHttpRequest(req);
        s->_idx = m_cnt;
        struct timeval t;
        gettimeofday(&t,NULL);
        s->_qid = t.tv_sec*1000000+t.tv_usec;
        s->_sendCnt = 0;

        /*生成请求报文*/
        snprintf(g_buf,1024*1024-1,"GET /%s?type=%s&lang=%s&qid=%ld&ptid=%s&auth=%s&ccy=%s&query=%s&refer_id=%s&cur_id=%s&next_id=%s&uid=%s&csuid=%s HTTP/1.0\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nContent-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n",
            s->_url_path.c_str(),s->_type.c_str(),s->_lang.c_str(),s->_qid/1000,s->_ptid.c_str(),s->_auth.c_str(),s->_ccy.c_str(),s->_query.c_str(),s->_refer_id.c_str(),s->_cur_id.c_str(),s->_next_id.c_str(),s->_uid.c_str(),s->_csuid.c_str());
        s->_msg = g_buf;
        
        if (g_debug)
            fprintf(stderr, "REQ<%d> qid:%ld\n", s->_idx, s->_qid);
        
        /*为请求创建socket*/
        int sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
        if(sk < 0) {   
            MJ_LOG_ERROR("[sock:%d]创建失败",sk);
            m_failCnt++;
            return;
        }
        s->_socket = sk;
        struct sockaddr_in sa = {0};   
        sa.sin_family = AF_INET;   
        sa.sin_port = htons(s->_port);   
        struct sockaddr_in *psa = &sa;   
        int iRet = inet_pton(AF_INET, s->_addr.c_str(), &psa->sin_addr.s_addr); 
        if (iRet <= 0){
            MJ_LOG_ERROR("[sock:%d]无效URL地址:[%s]",sk,s->_addr.c_str());
            m_failCnt++;
            close(s->_socket); 
            return;
        }
        //设置非阻塞
        setNonBlock(sk);
        //连接
        connect(sk, (struct sockaddr*)&sa, sizeof(sa));
        //注册监听事件
        addEpollOut(m_efds[m_cnt%m_efds.size()],s);
    
    }
private:
    void setNonBlock(int fd) {   
        int flag = fcntl ( fd, F_GETFL, 0 );   
        fcntl ( fd, F_SETFL, flag | O_NONBLOCK );   
    } 
    void addEpollOut(int efd, MyHttpRequest* session){
        //向epoll实例绑定要监控的fd及监控事件
        struct epoll_event event; 
        event.events = EPOLLOUT | EPOLLIN | EPOLLET;
        event.data.ptr = session;
        epoll_ctl(efd, EPOLL_CTL_ADD, session->_socket, &event); 
        return;
    }
    void modEpollIn(int efd, MyHttpRequest* session){
        struct epoll_event event; 
        event.events = EPOLLIN | EPOLLET;
        event.data.ptr = session;
        epoll_ctl(efd,EPOLL_CTL_MOD,session->_socket,&event);
        return;
    }
    void closeSession(MyHttpRequest* session, int epoll_fd, int stat){
        //断开连接
        if (session->_socket > 0){
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,session->_socket,NULL);
            close(session->_socket); 
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

};


MJLoader* loader = new MJLoader();


void doCtrlC(int sig){
    fprintf(stderr, "程序被中断，实际发送请求%d个, 正在等待已发送请求返回...\n",loader->m_cnt);

    while(!loader->isOver()){
        sleep(1);
    }
    loader->dump();

    loader->stop();
    exit(1);
}


int main(int argc, char** argv){

    signal(SIGINT,doCtrlC);

    fprintf(stderr, "测试用例请放到当前文件夹的case.list文件中\n用法:\n    ./mj_loader -i IP地址 -p 端口 -q QPS -t 测试秒数 -l 处理线程数 -d(开启详细日志)\n其中<-p>必填\n");
    int oc;
    while ((oc = getopt(argc, argv, "i:p:q:t:l:d")) != -1)  {  
        switch (oc)  
        {  
            case 'i':  
                g_addr = optarg;
                break;  
            case 'p':  
                g_port = atoi(optarg);
                break;  
            case 'q':  
                g_qps = atof(optarg);
                break;  
            case 't':  
                g_duration = atoi(optarg);
                break; 
            case 'l':  
                g_thread_num = atoi(optarg);
                break; 
            case 'd':  
                g_debug = 1;
                break;              
            default:  
                printf("unknown options\n");  
                exit(1);  
        }  
    }  
    if (g_port <=0)
        exit(1);
    
    std::vector<MyHttpRequest> cases;
    loadCases("case.list",cases);
    if (cases.size() == 0){
        cerr<<"无测试用例,请添加用例到case.list\n  极简示例:type=g105&ptid=ptid&query=%7B%22key%22%3A%22kai%27xuan%22%2C%22mode%22%3A%5B2%5D%2C%22num%22%3A20%2C%22filter%22%3A%7B%22citys%22%3A%5B%5D%2C%22cityType%22%3A1%7D%7D\n";
        exit(1);
    }
    cerr<<"加载测试用例"<<cases.size()<<"个"<<endl;

    
    loader->open(g_thread_num,50*1024*1024);
    loader->activate();

    sleep(1);

    int cnt = int(g_qps*g_duration);
    if (cnt==0)
        cnt = 1;
    size_t case_len = cases.size();
    fprintf(stderr, "开始发送请求，%zd个线程，QPS=%0.2f,共%d个请求\n", g_thread_num, g_qps, cnt);
    MyTimer c;
    
    int send_cnt = 0;
 
    struct timeval f,t;
    gettimeofday(&f,NULL);
    long start = f.tv_sec*1000000+f.tv_usec;
    // int tmp = 0;
    while(true){
        gettimeofday(&t,NULL);
        long now = t.tv_sec*1000000+t.tv_usec;
        int cnt_pos = 1 + int((double)(now - start)/1000000.0*g_qps);
        int pos = cnt_pos<cnt?cnt_pos:cnt;
        for (;send_cnt<pos;send_cnt++){
            // tmp++;
            loader->request(cases[(int)send_cnt%case_len]);
        }
        if (cnt_pos > cnt)
            break;
        usleep(100000);
    }
    // for(;send_cnt<cnt;send_cnt++){
    //     tmp++;
    //     loader->request(cases[(int)send_cnt%case_len]);
    // }

    fprintf(stderr, "请求发送完毕，用时%0.2f秒，实际发送请求%d个\n",(double)c.cost()/1000000,loader->m_cnt);



    while(!loader->isOver()){
        sleep(1);
    }
    loader->dump();

    loader->stop();
    return 0;
}