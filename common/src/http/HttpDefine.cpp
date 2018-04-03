#include "HttpDefine.h"
#include "AuxTools.h"

using namespace MJ;

const std::string HTTP_HEAD_END = "\r\n\r\n";

void MJHttpRequest::dump(){
    fprintf(stderr, "[REQ]:uri(%s) method(%s) type(%s) lang(%s) qid(%s) ptid(%s) auth(%s) ccy(%s) refer_id(%s) cur_id(%s) csuid(%s) uid(%s) query(%s)\n", 
        _uri.c_str(),_method.c_str(),_type.c_str(),_lang.c_str(),_qid.c_str(),_ptid.c_str(),_auth.c_str(),_ccy.c_str(),_refer_id.c_str(),_cur_id.c_str(),_csuid.c_str(),_uid.c_str(),_query.c_str());
}

void MJHttpRequest::parseParams(){
    size_t pos = 0;
    std::string key;
    std::string val;
    size_t len = _data_str.length();
    while(pos < len){
        size_t t = _data_str.find("=",pos);
        if (t == std::string::npos)
            break;
        key = _data_str.substr(pos,t-pos);
        pos = t+1;
        t = _data_str.find("&",pos);
        if (t == std::string::npos){
            val = UrlDecode(_data_str.substr(pos));
            _data[key] = val;
            break;
        }else{
            val = UrlDecode(_data_str.substr(pos,t-pos));
            pos = t+1;
            _data[key] = val;
        }
    }
    return;
}

void MJHttpRequest::parseREQLine(const std::string& request){
	size_t pos,pos1,pos2;
    //_method
    pos = request.find(" ");
    if (pos == std::string::npos)
        return;
    _method = request.substr(0,pos);
    //请求本体
    pos += 1;
    pos1 = request.find(" HTTP/",pos);
    if (pos1 == std::string::npos)
        return;
    pos2 = request.find("?",pos);
    if (pos2 == std::string::npos || pos2>pos1){
        _uri = request.substr(pos,pos1-pos);
    }else{
        _uri = request.substr(pos+1,pos2-pos-1);
		_data_str = request.substr(pos2+1,pos1-pos2-1);
        parseParams();
    }

    //_ver
    pos1 += 6;
    pos2 = request.find("\r\n",pos1);
    _ver = request.substr(pos1,pos2-pos1);

    return;
}

void MJHttpRequest::parseREQHeader(const char* header,char* val){
    if (!header){
        return;
    }
    char key[20];
    char tmp[5];
    std::map<std::string,std::string> kv;
    while(true){
        const char* end = strstr(header,"\r\n");
        if (!end){
            break;
        }
        sscanf(header,"%[^ :]%[ :]%s\r\n",key,tmp,val);
        kv[key] = val;
        header = end+2;
    }
    std::map<std::string,std::string>::iterator it = kv.begin();
    for (;it!=kv.end();it++){
        // printf("key:[%s] val:[%s]\n", it->first.c_str(),it->second.c_str());
        if (strcasecmp(it->first.c_str(),"Accept-Encoding")==0){
            _zip = it->second;
        }else if(strcasecmp(it->first.c_str(),"Content-Length")==0){
            _content_len = atoi(it->second.c_str());
        }
    }

    return;
}

int MJHttpRequest::getKeyInfo(){
    std::tr1::unordered_map<std::string,std::string>::iterator it;

    it = _data.find("type");
    if (it != _data.end()){
        _type = it->second;
        _data.erase("type");
    }

    it = _data.find("lang");
    if (it != _data.end()){
        _lang = it->second;
        _data.erase("lang");
    }

    it = _data.find("qid");
    if (it != _data.end()){
        _qid = it->second;
        _data.erase("qid");
    }

    it = _data.find("ptid");
    if (it != _data.end()){
        _ptid = it->second;
        _data.erase("ptid");
    }

    it = _data.find("auth");
    if (it != _data.end()){
        _auth = it->second;
        _data.erase("auth");
    }

    it = _data.find("ccy");
    if (it != _data.end()){
        _ccy = it->second;
        _data.erase("ccy");
    }

    it = _data.find("csuid");
    if (it != _data.end()){
        _csuid = it->second;
        _data.erase("csuid");
    }

    it = _data.find("uid");
    if (it != _data.end()){
        _uid = it->second;
        _data.erase("uid");
    }

    it = _data.find("query");
    if (it != _data.end()){
        _query = it->second;
        _data.erase("query");
    }

    _data.erase("refer_id");

    it = _data.find("cur_id");
    if (it != _data.end()){
        _refer_id = it->second;
        _data.erase("cur_id");
    }

    it = _data.find("next_id");
    if (it != _data.end()){
        _cur_id = it->second;
        _data.erase("next_id");
    }

    return 0;
}

int MJHttpRequest::parseREQ(const std::string& request){
    size_t pos;
    if (_stat == 0){
        pos = request.find(HTTP_HEAD_END);
        if (pos==std::string::npos)
            return _stat;
        //request line
        char buff[pos];
        parseREQLine(request);
        //header
        pos = request.find("\r\n");
        parseREQHeader(request.c_str()+2,buff);
        _stat = 1;
        if (_method == "GET"){
            _stat = 2;
            //解析部分MJ业务关键信息
            getKeyInfo();
        }
    }
    if (_stat == 1){
        //待添加 解压缩 content_length url_decode等处理
    }
    
    

    return _stat;
}


MJHttpRequest::MJHttpRequest(){
    _stat = 0;
    _method = "GET";
    _ver = "1.0";
    _zip = "";
    _uri = "";
    _data.clear();
    _content_len = -1;

    _type = "";
    _lang = "";
    _qid = "";
    _ptid = "";
    _auth = "";
    _ccy = "";
    _query = "";
    _refer_id = "api";
    _cur_id = "";
    _csuid = "";
    _uid = "";
}
MJHttpRequest::~MJHttpRequest(){

}
