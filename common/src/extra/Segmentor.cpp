#include "Segmentor.h"
#include "../ServiceLog.h"
using namespace std;

bool Segmentor::segment2unigram(const std::string& str, std::vector<std::string>& word_vec)
{
    word_vec.clear();
    for(int i = 0; i < str.size();)
    {
        //cout << i << endl;
        unsigned char c = str[i];
        string word = "";
        // 6 bytes
        if(c <= 32)
        {
            i += 1;
        }
        else if((c & 0xFC) == 0xFC)
        {
            word_vec.push_back(str.substr(i, 6));
            i += 6;
        }
        // 5 bytes
        else if((c & 0xF8) == 0xF8)
        {
            word_vec.push_back(str.substr(i, 5));
            i += 5;
        }
        // 4 bytes
        else if((c & 0xF0) == 0xF0)
        {
            word_vec.push_back(str.substr(i, 4));
            i += 4;
        }
        // 3 bytes
        else if((c & 0xE0) == 0xE0)
        {
            word_vec.push_back(str.substr(i, 3));
            i += 3;
        }
        // 2 bytes
        else if((c & 0xC0) == 0xC0)
        {
            word_vec.push_back(str.substr(i, 2));
            i += 2;
        }
        else
        {
		int j=i;
		while((c & 0xC0) != 0xC0 && c > 32)
		{
			j++;
			if(j>=str.size())
				break;
			c = str[j];
		}
		word = str.substr(i,j-i);
		word_vec.push_back(seg_st.toLowerCase(word));
		i=j;
        }
    }
}

bool Segmentor::gbksegment2unigram(const std::string& str, std::vector<std::string>& word_vec)
{
    word_vec.clear();
    for(int i = 0; i < str.size();++i)
    {
        //cout << i << endl;
        unsigned char c = str[i];
        string word = str.substr(i, 1);
        if(c > 127)
        {
            if(i + 1 >= str.size())
            {
                word_vec.clear();
                _ERROR("[gbksegment2unigram error, str = %s]", str.c_str());
                return false;
            }
            word += str.substr(i+1, 1);
            i+=1;
        }
        word_vec.push_back(word);
    }
    return true;
}


bool Segmentor::unigram2bigram(std::vector<std::string>& unigram_vec, std::vector<std::string>& bigram_vec)
{
    bigram_vec.clear();
    if(unigram_vec.size() <= 0)
        return false;
    if(unigram_vec.size() == 1)
        bigram_vec.push_back(unigram_vec[0]);
    for(int i = 0; i < unigram_vec.size() - 1; ++i)
        bigram_vec.push_back(unigram_vec[i] + unigram_vec[i+1]);
}


