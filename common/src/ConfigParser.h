#ifndef _CONFIGPARSER_HPP_
#define _CONFIGPARSER_HPP_
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace MJ
{

typedef std::map<std::string, std::map<std::string, std::string> > CONFIG;

class ConfigParser
{
    //friend std::ostream& operator << (std::ostream& out, const CONFIG& config);
    public:
        ConfigParser();
        ~ConfigParser();

        //读取标准格式配置文件，返回值为map<string, map<string, string> >
        CONFIG read(const std::string& filepath);
        //传入一个CONFIG格式的数据，写入文件filename中，如果写入成功返回true
        bool write(const CONFIG& content, const std::string& filepath);

        //读取标准格式配置文件，返回值为vector<string>
        std::vector<std::string> readMod2(const std::string& filepath);
        //传入一个vector<string>格式的数据，写入文件filename中，如果写入成功返回true
        bool write(const std::vector<std::string>& content, const std::string& filepath);

    private:
        static std::ifstream& openfileRead(std::ifstream& is, const std::string& filename);
        static std::ofstream& openfileWrite(std::ofstream& os, const std::string& filename);
        //去掉字符串首位空白符
        static void trim(std::string& s);
        //切割字符串
        static void split(const std::string& str, const std::string& delim, std::vector<std::string>& vec);
};

//重载输出操作符
std::ostream& operator << (std::ostream& out, const std::map<std::string, std::string>& smap);
std::ostream& operator << (std::ostream& out, const CONFIG& config);
std::ostream& operator << (std::ostream& out, const std::vector<std::string>& config);

}

#endif  /*_CONFIGPARSER_HPP_*/
