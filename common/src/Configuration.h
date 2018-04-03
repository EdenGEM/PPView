#ifndef __CONFIGURATION_HPP__
#define __CONFIGURATION_HPP__

#include <string>

namespace MJ
{

class Configuration {
	public:
		Configuration(): m_needDbg(1), m_needLog(1), m_needErr(1), m_needDump(0) {
		}
		int open(const char *filename);
	private:
		void analyzeLine(const std::string& line);
	public:
		unsigned int m_listenPort;
		unsigned int m_threadStackSize;

		unsigned int m_receiverNum;
		unsigned int m_processorNum;

		int m_commandLengthWarning; //qo 的result结果长度最长为4k 因为此处是人工编辑 所以限定最大长度 否则可能被丢弃结果
		std::string m_dataPath;

		int m_needDbg;
		int m_needLog;
		int m_needErr;
		int m_needDump;
};

}

#endif /* __CONFIGURATION_HPP__ */
