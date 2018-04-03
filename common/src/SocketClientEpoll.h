#ifndef __SOCKET_CLIENT_EPOLL__
#define __SOCKET_CLIENT_EPOLL__



namespace MJ{

class SocketClientEpoll{
public:
	static void setNonBlock(int fd);
	static int doHttpRequest(const std::string& addr, 
                             const int port,
                             const std::string& query,
                             char* respBuffer,
                             int& respBufferSize,
                             int timeout = -1);

};

}       //namespace MJ






#endif		//__SOCKET_CLIENT_EPOLL__

