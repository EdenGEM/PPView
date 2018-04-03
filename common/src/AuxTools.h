#ifndef AUXTOOLS_H
#define AUXTOOLS_H

#include "time.h"
#include <sys/time.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "openssl/md5.h"
#include <string.h>
#include <stdio.h>
#include "json/json.hpp"

const std::string g_commonVersion = "v1.7.1";

namespace MJ
{

#define EARTH_RADIUS 6378137
#define PI 3.1415927

std::string getVersion();
long getTimestampUs();
long getTimestampMs();
double getSphereDist(double lng1,double lat1,double lng2,double lat2);
bool md5(const std::string& str, std::string& md5_result);
bool md5Last4(const std::string& str, int& md5_int);
//此url编解码，参照官方标准实现:https://en.wikipedia.org/wiki/Percent-encoding
std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);

int tokenize(const std::string &src,std::vector<std::string> &tokens,const std::string &delim);
int join(const std::vector<std::string> & val, const std::string & sep,std::string & ret);

//获取本机IP地址
std::string getLocalIP();
//获取进程绝对路径
std::string getProcessPath();
//获取进程名称
std::string getProcessName();



/*gzip压缩相关*/
// GZip Compression
// @param data - 待压缩的字符串
// @param compressedData - 压缩后的数据
// @param level - the gzip compress level -1 = default, 0 = no compression, 1= worst/fastest compression, 9 = best/slowest compression
// @return - true on success, false on failure
bool compress_gzip(const std::string& data, std::string& compressedData, int level = -1);
// GZip Decompression
// @param compressedData - 待解压的数据
// @param data - 解压后的数据
// @return - true on success, false on failure
bool decompress_gzip(const std::string& compressedData, std::string& data);


//该类多线程不安全
class StaticRand {
    public:
        StaticRand() {
            m_rlist = NULL;

        }
        ~StaticRand() {

        }
    public:
        int Max() {
            return m_max;
        }

        int Capacity() {
            return m_capacity;
        }

        int Get(int index) {
            return m_rlist[index % m_capacity];
        }

        int Release() {
            if (m_rlist) {
                delete[] m_rlist;
                m_rlist = NULL;
            }
            return 0;
        }

        int Init() {
            Release();
            m_rlist = new int[m_capacity];

            srand(1);
            for (int i = 0; i < m_capacity; ++i) {
                m_rlist[i] = rand();
            }
            return 0;
        }

    private:
        static int* m_rlist;
        static const int m_capacity = 10000000;
        static const int m_max = RAND_MAX;

};



}
#endif
