#include "Configuration.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>


using namespace std;
using namespace MJ;

void Configuration::analyzeLine(const std::string& line){
	size_t pos = line.find("=");
	if (pos==std::string::npos)
		return;
	string key = line.substr(0,pos);
	string val = line.substr(pos+1);
	if (key == "ListenPort"){
		m_listenPort = atoi(val.c_str());
	} else if (key=="ThreadStackSize"){
		m_threadStackSize = atoi(val.c_str());
	} else if (key=="CommandLengthWarning"){
		m_commandLengthWarning = atoi(val.c_str());
	} else if (key=="ReceiverThread"){
		m_receiverNum = atoi(val.c_str());
	} else if (key=="ProcessorThread"){
		m_processorNum = atoi(val.c_str());
	} else if (key=="DataPath"){
		m_dataPath = val;
	} else if (key == "NeedDbg") {
		m_needDbg = atoi(val.c_str());
	} else if (key == "NeedLog") {
		m_needLog = atoi(val.c_str());
	} else if (key == "NeedErr") {
		m_needErr = atoi(val.c_str());
	} else if (key == "NeedDump") {
		m_needDump = atoi(val.c_str());
	}
	return;

}

int Configuration::open(const char *filename){
	ifstream fin;
	fin.open(filename);
	if (!fin){
		cerr<<"Open File "<<filename<<" failed!"<<endl;
		return 1;
	}

	string line = "";
	while(!fin.eof()){
		getline(fin,line);
		if (line.length()==0 || line[0]=='#')
			continue;
		analyzeLine(line);
	}
	return 0;
}



