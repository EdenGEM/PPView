#include "ConfigParser.h"

using namespace std;
using namespace MJ;

ConfigParser::ConfigParser()
{
}

ConfigParser::~ConfigParser()
{
}

ifstream& ConfigParser::openfileRead(ifstream& is, const string& filename)
{
    is.close();
    is.clear();
    is.open(filename.c_str());

    return is;
}

ofstream& ConfigParser::openfileWrite(ofstream& os, const string& filename)
{
    os.close();
    os.clear();
    os.open(filename.c_str(), ofstream::out);
    return os;
}

//去掉字符串首尾空格
void ConfigParser::trim(string& s)
{
    if (s.empty())
        return;
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);

    return;
}

//以delim分割字符串str，分割好的结果保存到vec中去
void ConfigParser::split(const string& str, const string& delim, vector<string>& vec)
{
    vec.clear();    //清空vector
    size_t beg = 0, end;
    size_t delim_len = delim.size();
    do
    {
        string word = "";
        end = str.find(delim, beg);
        if (end == std::string::npos)
        {
            word = str.substr(beg);
            vec.push_back(word);
            break;
        }
        else
        {
            word = str.substr(beg, end - beg);
            vec.push_back(word);
        }
        beg = end + delim_len;
        if (beg == str.size())
        {
            vec.push_back("");
            break;
        }
    }while (1);
}

bool ConfigParser::write(const vector<string>& content, const string& filepath)
{
    ofstream os;
    if (!openfileWrite(os, filepath))
    {
        cout << "open configFile " << filepath << " failed!" << endl;
        return false;
    }
    for (vector<string>::const_iterator it = content.begin(); it != content.end(); ++it)
    {
        os << *it << endl;
    }
    return true;
}

bool ConfigParser::write(const CONFIG& content, const string& filepath)
{
    ofstream os;
    if (!openfileWrite(os, filepath))
    {
        cout << "open configFile " << filepath << " failed!" << endl;
        return false;
    }
    for (CONFIG::const_iterator it = content.begin(); it != content.end(); ++it)
    {
        os << "[" << it->first << "]" << endl;
        for (map<string, string>::const_iterator m_it = it->second.begin(); m_it != it->second.end(); ++m_it)
        {
            os << m_it->first << "=" << m_it->second << endl;
        }
        os << endl;
    }
    return true;
}

CONFIG ConfigParser::read(const string& filepath)
{
    CONFIG results;
    ifstream is;
    if (!openfileRead(is, filepath))
    {
        cout << "open configFile " << filepath << " failed!" << endl;
        return results;
    }
    string line = "";
    string cur_key = "";    //标记当前key
    while (getline(is, line))
    {
        trim(line);  //去掉首尾空格
        if (line[0] == '[')
        {
            //添加key
            int beg = 1, end = 0;
            if ((end = line.find(']')) != -1)
            {
                cur_key = line.substr(beg, end - beg);
                map<string, string> temp_value;
                results[cur_key] = temp_value;
            }
            else
            {
                continue;    //此行不符合格式要求，直接跳过
            }
        }
        else if (line.find('=') != std::string::npos && cur_key != "")
        {
            string key = line.substr(0, line.find('='));
            trim(key);
            string value = line.substr(line.find('=') + 1);
            trim(value);
            if (key == "")
                continue;
            results[cur_key][key] = value;
        }
        else        //不符合要求的行，直接跳过
        {
            continue;
        }
    }

    return results;
}

//读取模式2，直接将配置文件的每一行保存到vector中保存然后返回
vector<string> ConfigParser::readMod2(const string& filepath)
{
    vector<string> lines;
    ifstream is;
    if (!openfileRead(is, filepath))
    {
        cout << "open configFile " << filepath << " failed!" << endl;
        return lines;
    }
    string line = "";
    while (getline(is, line))
    {
        trim(line);
        lines.push_back(line);
    }
    return lines;
}

namespace MJ{

ostream& operator << (ostream& out, const map<string, string>& smap)
{
    out << "{";
    if (smap.size() >= 1)
    {
        for (map<string, string>::const_iterator mit = smap.begin(); mit != smap.end(); ++mit)
        {
            out <<  mit->first + ":" + mit->second + ",";
        }
        out << "\b";
    }
    out << "}";

    return out;
}

ostream& operator << (ostream& out, const CONFIG& config)
{
    out << "{";
    if (config.size() >= 1)
    {
        for (CONFIG::const_iterator cit = config.begin(); cit != config.end(); ++ cit)
        {
            out << cit->first << ":" << (cit->second) << ",";
        }
        out << "\b";
    }
    out << "}";

    return out;
}

ostream& operator << (ostream& out, const vector<string>& svec)
{
    out << "[";
    if (svec.size() > 1)
    {
        for (vector<string>::const_iterator vit = svec.begin(); vit != svec.end(); ++vit)
        {
            out << *vit << ",";
        }
        out << "\b";
    }
    out << "]";

    return out;
}

}

