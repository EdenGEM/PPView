#include "AuxTools.h"
#include <assert.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <zlib.h>

using namespace std;

namespace MJ
{
#define CHUNK 16384

std::string getVersion(){
  return g_commonVersion;
}

long getTimestampUs()
{
	timeval t;
	gettimeofday(&t,NULL);
	return t.tv_sec*1000*1000 + t.tv_usec;
}

long getTimestampMs()
{
	timeval t;
	gettimeofday(&t,NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
}

int tokenize(const std::string &src,std::vector<std::string> &tokens,const std::string &delim)
{
    std::string::size_type pos1=0,pos2=0;
    while (true)
    {   
        pos1=src.find(delim,pos2);

        if(pos1==std::string::npos){
            string tmp = src.substr(pos2);
            tokens.push_back(tmp);
            break;
        }else{
            string tmp = src.substr(pos2,pos1-pos2);
            tokens.push_back(tmp);
        }   
        pos2=pos1+delim.size();
    }   
    return tokens.size();
}

int join(const std::vector<std::string> & val, const std::string & sep,std::string & ret)
{
    if(val.size()==0)return 0;
    std::ostringstream oss;
    oss<<val[0];
    for(size_t i=1;i<val.size();i++)
        oss<< sep<<val[i];
    ret=oss.str();
    return 0;
}

double getSphereDist(double lng1,double lat1,double lng2,double lat2)
{
	double radlat1 = lat1 * PI / 180.0;
	double radlat2 = lat2 * PI / 180.0;
	double a = radlat1 - radlat2;
	double b = (lng1 - lng2) * PI / 180.0;
	double s = 2 * asin(sqrt(pow(sin(a/2),2) + cos(radlat1)*cos(radlat2)*pow(sin(b/2),2))) * EARTH_RADIUS;
	return s;
}

bool md5(const string& str, string& md5_result)
{
	char buff[33]={'\0'};
	unsigned char md5[16] = {0};
	char tmp[3]={'\0'};

	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, str.c_str(), str.size());
	MD5_Final(md5, &ctx);

	for(int i = 0; i<16; i++)
	{
		sprintf(tmp, "%02x", md5[i]);
		strcat(buff, tmp);
	}
	md5_result = buff;
	return true;
}

bool md5Last4(const std::string& str, int & md5_int)
{
	unsigned char md5[16] = {0};

	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, str.c_str(), str.size());
	MD5_Final(md5, &ctx);

	md5_int = md5[14];
	md5_int *= 16*16;
	md5_int += md5[15];
	return true;
}

unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y = '\0';
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%' && i + 2 < length && isxdigit((int)str[i+1]) && isxdigit((int)str[i+2]))
        {
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

std::string getLocalIP(){
    int i=0;
    int sockfd;
    struct ifconf ifconf;
    char buf[512];
    struct ifreq *ifreq;
    char* ip;
    //初始化ifconf
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = buf;
 
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0){
        return "";
    }
    ioctl(sockfd, SIOCGIFCONF, &ifconf);    //获取所有接口信息
    close(sockfd);
    //接下来一个一个的获取IP地址
    ifreq = (struct ifreq*)buf;
 
    for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i>0; i--){
        ip = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
        //排除127.0.0.1，继续下一个
        if(strcmp(ip,"127.0.0.1")==0){
            ifreq++;
            continue;
        }
        return std::string(ip);
    }
 
    return "";
}

//获取进程绝对路径
std::string getProcessPath(){
    char path[1024];
    if(readlink("/proc/self/exe", path,1023) <=0)
        return "";
    return std::string(path);

}
//获取进程名称
std::string getProcessName(){
    std::string path = getProcessPath();
    size_t pos = path.rfind("/");
    if (pos != std::string::npos)
        return path.substr(pos+1);
    else
        return path;
}



bool compress_gzip(const std::string& data, std::string& compressedData, int level){
  compressedData = "";
  unsigned char out[CHUNK];
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  if (deflateInit2(&strm, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
  {
    return false;
  }
  strm.next_in = (unsigned char*)data.c_str();
  strm.avail_in = data.size();
  do {
    int have;
    strm.avail_out = CHUNK;
    strm.next_out = out;
    if (deflate(&strm, Z_FINISH) == Z_STREAM_ERROR)
    {
      return false;
    }
    have = CHUNK - strm.avail_out;
    compressedData.append((char*)out, have);
  } while (strm.avail_out == 0);
  if (deflateEnd(&strm) != Z_OK)
  {
    return false;
  }
  return true;
}

bool decompress_gzip(const std::string& compressedData, std::string& data){
  data = "";
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char out[CHUNK];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
  {
    return false;
  }

  strm.avail_in = compressedData.size();
  strm.next_in = (unsigned char*)compressedData.c_str();
  do {
    strm.avail_out = CHUNK;
    strm.next_out = out;
    ret = inflate(&strm, Z_NO_FLUSH);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      inflateEnd(&strm);
      return false;
    }
    have = CHUNK - strm.avail_out;
    data.append((char*)out, have);
  } while (strm.avail_out == 0);

  if (inflateEnd(&strm) != Z_OK) {
    return false;
  }
  
  return true;
}

}       //namespace MJ
