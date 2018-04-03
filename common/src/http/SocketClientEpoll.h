
#ifndef __SOCKET_CLIENT_EPOLL__
#define __SOCKET_CLIENT_EPOLL__

#include <string>
#include <sys/time.h>
#include <vector>


namespace MJ{

#define READ_BUF_SIZE_CLIENT (128*1024)

enum MJHttpMethod {
    Get = 0,
    Get_Http_11,
    Post
};

/*通用请求Session*/
class SocketSession{
public:
    /*通用必填的输入参数*/
    std::string _addr;
    int _port;
    int _timeout;  //超时时间 单位毫秒
    MJHttpMethod _method;   //请求方法
    std::string _url_path;  //请求的url path  后台请求一般为空串

    /*请求参数*/
    std::string _data;        //post请求的数据 或者 get请求的uri的“?”号后面的数据
    std::string _qid;       //客户端最初的请求ID 表示一个业务 客户端使用时间戳即可（精确到毫秒)
    std::string _refer_id;  //上游传来的请求ID
    std::string _cur_id;    //本服务的请求ID
    std::string _ip;        //打请求的服务IP

    /*中间变量*/
    int _socket;
    int _sendCnt;
    std::string _msg;    //实际发出的全部报文(含报头)
    timeval _create_time;    //new的时间点

    /*输出参数*/
    int _response_len;  //-1:表示请求未被处理完 0:请求失败(未知原因) >0:请求成功(含报头长度) -2:请求被断开 -3:请求超时 -4:socket创建失败 -5:无效IP
    std::string _response;  //实际收到的回报的报文 （不含报头）

public:
    SocketSession(){}
    SocketSession(const std::string& addr,
                int port,
                int timeout,
                MJHttpMethod method,
                const std::string& url_path,
                const std::string& data,
                const std::string& qid,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& ip);
    virtual ~SocketSession();
    virtual void makeHttpRequest();
    virtual void logRequest();
    virtual void logResponse();
};


/*妙计内部服务用的请求Session*/
class MJSocketSession: public SocketSession{
public:
    /*请求参数*/
    std::string _type;      //请求类型
    std::string _lang;      //语言版本 "en|zh_cn"
    std::string _ptid;      //合作方ID
    std::string _auth;      //合作方权限
    std::string _ccy;       //当前期望的货币类型 CNY HKD USD EUR  默认为CNY
    std::string _csuid;        //客服ID
    std::string _uid;        //终端用户ID
    std::string _query;     //json对象的字符串，每个接口的业务逻辑都在这里

    /*自动生成的参数，不需要传入*/
    std::string _next_id;      //下游请求ID 根据请求生成

public:
    MJSocketSession(const std::string& addr,
                int port,
                int timeout,
                MJHttpMethod method,
                const std::string& url_path,
                const std::string& type,
                const std::string& qid,
                const std::string& query,
                const std::string& ip="",
                const std::string& refer_id = "api",
                const std::string& cur_id = "",
                const std::string& ptid = "",
                const std::string& lang = "zh_cn",
                const std::string& ccy = "CNY",
                const std::string& csuid = "",
                const std::string& uid = "",
                const std::string& auth = "");
    virtual ~MJSocketSession();
    virtual void makeHttpRequest();
    void makeGetUrl(std::string& url);
    virtual void logRequest();
    virtual void logResponse();
};

class SocketClientEpoll{
public:
    /*妙计内部请求方法*/
    static int doHttpRequest(SocketSession* session);
    static int doHttpRequest(const std::vector<SocketSession*>& sessions,int timeout);
private:
    static void setNonBlock(int fd);
};

}       //namespace MJ






#endif		//__SOCKET_CLIENT_EPOLL__

