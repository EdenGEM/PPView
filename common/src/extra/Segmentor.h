#ifndef _SEGMENTOR_H_
#define _SEGMENTOR_H_

#include <iostream>
#include <string>
#include <vector>
#include "string/StringTools.h"


class Segmentor{
    public:
        Segmentor(){};
        ~Segmentor(){};
        bool segment2unigram(const std::string& str, std::vector<std::string>& word_vec);
        bool gbksegment2unigram(const std::string& str, std::vector<std::string>& word_vec);
        bool unigram2bigram(std::vector<std::string>& unigram_vec, std::vector<std::string>& bigram_vec);
	StrTools seg_st;
};

#endif
