
#ifndef __HTTP_DEFINE_H__
#define __HTTP_DEFINE_H__

#include <string>
#include <tr1/unordered_map>



#define CRLF "\r\n"

namespace MJ{

class MJHttpRequest
{
public:
    //通用参数
    int _stat;      //0:未接受到http报头 1:接受完报头，但未结束(POST请求未收完所有数据) 2:请求接收完毕
    std::string _method;
    std::string _ver;
    std::string _zip;
    std::string _uri;   //url中参数之前的部分
    std::string _data_str;  //请求参数的原始字符串（value部分urlencode）
    std::tr1::unordered_map<std::string,std::string> _data;  //请求的参数
    //MJ业务专用关键参数
    std::string _type;
    std::string _lang;
    std::string _qid;
    std::string _ptid;
    std::string _ccy;
    std::string _query;
    std::string _refer_id;
    std::string _cur_id;
    std::string _csuid;     //客服ID
    std::string _uid;       //终端用户ID
    std::string _auth;       //企业拥有的权限
    
	std::string _client_ip;


    //post相关参数
    size_t _content_len;    
    
public:
    MJHttpRequest();
    ~MJHttpRequest();
    int parseREQ(const std::string& request);
    void dump();
private:
    void parseParams();
    void parseREQLine(const std::string& request);
    void parseREQHeader(const char* header,char* val);
    int getKeyInfo();
};

}   //namespace MJ


#endif  //__HTTP_DEFINE_H__
