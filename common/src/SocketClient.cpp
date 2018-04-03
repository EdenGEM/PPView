#include "SocketClient.h"
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;
using namespace MJ;

typedef unsigned char BYTE;

const static int MAX_NUM = 1024*10;
const static int MAXSTRLEN = 1024;
const static int max_buf_len = 35*1024*1024;
#define SEG_BUF 50


template<class InT,class OutT>
class Transfer{
    public:
        static OutT convert(const InT& inValue){
            std::stringstream ss;
            ss<<inValue;
            OutT res;
            ss>>res;
            return res;

        }

};

bool SocketClient::init(const std::string& addr,const long& timeout){
    size_t pos = addr.find(":");
    if (pos==string::npos)
        return false;
    m_ip = addr.substr(0,pos);
	m_port = Transfer<string,int>::convert(addr.substr(pos+1));
	m_addr = addr;
	m_timeout = timeout;

    struct hostent *pHost,host;
    memset(&host,0, sizeof(hostent));
    int tmp_errno=0;
    char host_buff[2048];
    memset(host_buff,0, sizeof(host_buff));
    if (gethostbyname_r(m_ip.c_str(),&host, host_buff, sizeof(host_buff),&pHost, &tmp_errno)) {
        cerr<<"SocketClient::gethostbyname Failed!"<<endl;
        return false;
    }else{
        if (pHost && AF_INET == pHost->h_addrtype) {
            memcpy(&m_real_addr, pHost->h_addr_list[0], sizeof(m_real_addr));
        } else {
            cerr<<"SocketClient::host create failed!"<<endl;
            return false;
        }
    }
    return true;
}

int SocketClient::read_timeout(int fd, char* buf ,int len, timeval *timeout)
{
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(fd, &wset);
    if ((select(fd + 1, &wset, NULL, NULL, timeout)) <= 0) {
        return -1;
    }
    return read(fd, buf, len);
}


int SocketClient::readn_timeout(int fd, char* content, int need_to_read, timeval *timeout)
{
    char buf[SEG_BUF + 1];
    int n, left;
    int len;
    int ptr = 0;
    left = need_to_read;
    while (left > 0) {
        len = left > SEG_BUF ? SEG_BUF : left;
        n = read_timeout(fd, buf, len, timeout);
        if (n <= 0 ) {
            return need_to_read - left;
        }
        buf[n] = '\0';
        memcpy(content + ptr, buf, len);
        ptr = ptr + n;
        left = left - n;
    }
    return 0;
}


int SocketClient::read_http_header_timeout(int fd,char* content, timeval *timeout)
{
    char buf[SEG_BUF + 1];
    int n;
    int len = SEG_BUF;
    int ptr = 0;
    content[0] = '\0';
    while (1) {
        n = read_timeout(fd, buf, len, timeout);
        if (n <= 0 ) {
            return 0;
        }
        buf[n] = '\0';

		if (ptr + n + 1 >= max_buf_len)
		{
			return 0;
		}

		memcpy(content + ptr, buf, n+1);
        ptr = ptr + n;

		if (ptr >= max_buf_len)
			return 0;

    }
    return 0;
}

//返回报文正文的起始位置
int SocketClient::recv_result(char* result, int& len, int fd, long time_out_us)
{
    int ret;
    len = 0;
    struct timeval timeout = {0, time_out_us};
    ret = read_http_header_timeout(fd, result, &timeout);
    if (ret < 0) {
        return -1;
    }
    char* head_ptr = strstr(result,"\r\n\r\n");
    int tail_pos = strlen(result);
    if (head_ptr==NULL){
        len = tail_pos;
        return 0;
    }else{
        *head_ptr = '\0';
        ret = (head_ptr-result)+4;
        len = tail_pos - ret;
        return ret;
    }
}

bool SocketClient::string_split(const string& str, const string& pattern, vector<string>& v)
{
    v.clear();
    size_t bpos = 0;

    while(str.find(pattern, bpos) != std::string::npos)
    {
        size_t epos = str.find(pattern, bpos);
        if(epos == 0)
        {
            bpos = epos + pattern.size();
            continue;
        }
        v.push_back(str.substr(bpos, epos - bpos));
        bpos = epos + pattern.size();
        if(bpos >= str.size())
            break;
    }

    if(bpos < str.size())
        v.push_back(str.substr(bpos, str.size() - bpos));
    return true;
}
bool SocketClient::getRstFromHost(const std::string& query,ServerRst& sr,unsigned short type)
{
	int sock;
    struct sockaddr_in server;

	cout.flush();
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("opening stream socket");
        return  false;
    }
    server.sin_family = AF_INET;
    /*
    //DNS缓存 CACHE查询
	string cache_key = "DNS:" + m_ip;
	string cache_val="";
	std::map<std::string,std::string>::iterator it = m_cache.find(cache_key);
	if (it!=m_cache.end()){
		cerr<<"DNS Cached"<<endl;
		server.sin_addr.s_addr = Transfer<string,unsigned int>::convert(cache_val);
	}else{
        struct hostent *pHost,host;
        memset(&host,0, sizeof(hostent));
        int tmp_errno=0;
        char host_buff[2048];
        memset(host_buff,0, sizeof(host_buff));
		if (gethostbyname_r(m_ip.c_str(),
					&host, host_buff, sizeof(host_buff),
					&pHost, &tmp_errno)) {
			cerr<<"get host Fail!!!"<<endl;
			return false;
		} else {
			if (pHost && AF_INET == pHost->h_addrtype) {
				memcpy(&server.sin_addr, pHost->h_addr_list[0], sizeof(server.sin_addr));
				m_cache.insert(make_pair(cache_key,Transfer<unsigned int,string>::convert(server.sin_addr.s_addr)));
			} else {
				cerr<<"host create Fail!!!"<<endl;
				return false;
			}
		}
	}
    */
    server.sin_addr = m_real_addr;

    server.sin_port = htons(m_port);
    int ret;

    unsigned long ul = 1;
    ioctl(sock, FIONBIO, &ul);//设置为非阻塞模式
	fd_set fdset;
	bool bSucc = false;
	struct timeval tm = {0, m_timeout};

	cout.flush();
    if ((ret = connect(sock, (struct sockaddr*)&server, sizeof(server))) < 0) {
        //设置连接超时
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        if (select(sock+1, NULL, &fdset, NULL, &tm) > 0)
			bSucc = true;
	    else{
			bSucc = false;
		}
    }
	else{
		bSucc = true;
	}

     ul = 0;
     ioctl(sock, FIONBIO, &ul);//设置为阻塞模式
     if (!bSucc)
     {
               close(sock);
               cerr<<"ERROR:connecting stream socket"<<endl;
               fprintf(stderr,"err:%d(%s),ret:%d\n",errno,strerror(errno),ret);
               fprintf(stderr,"EINPROGRESS:%d, EALREADY:%d\n",EINPROGRESS,EALREADY);
               return false;
     }


    ostringstream oss_qlen;
    oss_qlen << query.size();

	string use_host = "default";
	string sendmsg_str;
	if (type==0){
        sendmsg_str = (string)"GET /" + query + " HTTP/1.0\r\n"+
				(string)"Accept: */*\r\n" +
    	        //(string)"Accept-Language: zh-cn\r\n" +
				(string)"Host: " + use_host + "\r\n\r\n";
				//(string)"Content-Length: "+ oss_qlen.str() + (string)" \r\n"+
				//(string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n";
	}else if (type==2){
        sendmsg_str = (string)"GET /" + query + " HTTP/1.1\r\n"+
				(string)"Accept: */*\r\n" +
    	        (string)"Accept-Language: zh-cn\r\n" +
				(string)"Host: " + use_host + "\r\n"+
				(string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n";
	}
	else if (type==1){
		sendmsg_str = (string)"POST /" + " HTTP/1.1\r\n"+
			(string)"Accept: */*\r\n" +
			(string)"Accept-Language: zh-cn\r\n" +
			(string)"Host: " + use_host + "\r\n"+
			(string)"Content-Type: application/x-www-form-urlencoded;charset=gbk\r\n";
			(string)"Content-Length: "+ oss_qlen.str() + (string)" \r\n\r\n"+ query;
	}


	cout.flush();
    if (send(sock, sendmsg_str.c_str(), sendmsg_str.size(), 0) < 0)
    {
        perror("sending on stream socket");
        fprintf(stderr,"err:%d(%s)",errno,strerror(errno));
        close(sock);
        return false;
    }

	cout.flush();
    char* rcvbuf = new char[max_buf_len];
    memset(rcvbuf, 0, max_buf_len);
    int buf_len=max_buf_len;
    int rt = recv_result(rcvbuf, buf_len, sock,m_timeout);
    if(rt < 0)
    {
        printf("Return Error:%d\n",rt);
        fprintf(stderr, "Socket Timeout or Error\n");
        close(sock);
        delete[] rcvbuf;
        return false;
    }

    sr.ret_len = buf_len;
    //对报文做转码
    decodeBody(sr.ret_str,rcvbuf,rt);
    cout.flush();

    delete[] rcvbuf;
    close(sock);
    return true;
}

void SocketClient::decodeBody_Chunked(std::string& ret,char* buf){
    char* p = buf;
    char t;
    int len = strlen(buf);
    while ((p-buf)<len){
        //获取chunk长度
        char* stop;
        int sz = strtol(p,&stop,16);
        if (!sz)
            return;
        //添加body内容
        p = stop+2;
        t = *(p+sz);
        *(p+sz) = '\0';
        ret += p;
        *(p+sz) = t;
        p = p+sz+2;
    }
    return;
}

void SocketClient::decodeBody(std::string& ret,char* buf,int body_pos){
    ret = "";
    if (strstr(buf,"Transfer-Encoding: chunked")==NULL){
        ret = buf+body_pos;
    }else{
        decodeBody_Chunked(ret,buf+body_pos);
    }

    return;
}


