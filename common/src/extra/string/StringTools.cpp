#include "StringTools.h"
using namespace std;
// 分割字符串
int StrTools::join(const std::string& sep,std::string & ret,std::vector<std::string>& v)
{
	if(v.size()==0)return 0;
	ret+=v[0];
	for(int i=1;i<v.size();i++)
	{
		ret+=sep;
		ret+=v[i];
	}
	return 0;
}
int StrTools::split(const string& str, const string& pattern, vector<string>& v)
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

    return v.size();
}

// 取特定子串
string StrTools::substr(const string& s, const string& begin_pattern, const string& end_pattern)
{
    string retstr = "";
    size_t bpos = s.find(begin_pattern);
    size_t epos = 0;

    if(bpos != string::npos)
    {   
        bpos += begin_pattern.size();
        if(bpos >= s.size())
            return retstr;

        epos = s.find(end_pattern, bpos);
        if(epos != string::npos)
            retstr = s.substr(bpos, epos - bpos);
        else
            retstr = s.substr(bpos, s.size() - bpos);
    }
    return retstr;
}

string StrTools::toLowerCase(const string& s)
{
    string ret = s;
    for(int i = 0; i < ret.size(); ++i)
    {
        if((int)ret[i] >= 65 and (int)ret[i] <= 90)
            ret[i] += 32;
    }
    return ret;
}

string StrTools::toUpperCase(const string& s)
{
    string ret = s;
    for(int i = 0; i < ret.size(); ++i)
    {
        if((int)ret[i] >= 97 and (int)ret[i] <= 122)
            ret[i] -= 32;
    }
    return ret;
}

string StrTools::trim(const string& s)
{
	string ret = s;
	if(s.size()==0)
		return ret;
	int i=0;
	for ( ; i < s.size(); i++)
	{
		if(s[i]!=' ' && s[i]!='\t' && s[i]!='\r' && s[i]!='\n' && s[i]!='\v' && s[i]!='\f')
			break;
	}
	int j=s.size()-1;
	for ( ; j >= i ;j--)
	{
		if(s[j]!=' ' && s[j]!='\t' && s[j]!='\r' && s[j]!='\n' && s[j]!='\v' && s[j]!='\f')
			break;
	}
	if(j>=i)
		ret = s.substr(i,j-i+1);
	else
		ret = "";
	return ret;
}
