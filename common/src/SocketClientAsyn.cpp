#include "SocketClientAsyn.h"


using namespace std;
using namespace MJ;

int SocketClientAsyn::init(const std::string &addr, time_t timeout, size_t buffSize) {
	//写入原始数据
	m_addr = addr;
	m_timeout = timeout;
	m_buffSize = buffSize;

	//IP和端口转换
	size_t pos = addr.find(":");
	if(pos == std::string::npos) {
		std::cerr << "[Addr ERROR]:" << addr << endl;
		return -1;
	}
	m_port = boost::lexical_cast<int>(addr.substr(pos+1));
	string taddr = addr.substr(0, pos);

	struct hostent *pHost, host;
	memset(&host, 0, sizeof(hostent));
	int tmp_errno = 0;
	char hostBuff[2048];
	memset(hostBuff, 0, sizeof(hostBuff));
	if(gethostbyname_r(taddr.c_str(), &host, hostBuff, sizeof(hostBuff),&pHost, &tmp_errno)) {
		cerr << "SocketClientAsyn::gethostbyname Failed!" << endl;
		return -1;
	}
	else {
		if(pHost && AF_INET == pHost->h_addrtype) {
			memcpy(&m_realAddr, pHost->h_addr_list[0], sizeof(m_realAddr));
		}
		else {
			cerr << "SocketClientAsyn::host create failed!" << endl;
			return -1;
		}
	}
	return 0;
}

int SocketClientAsyn::doRequest(const std::vector<std::string> &queryList, std::vector<std::string> &respon, int type) {
	if(queryList.size() == 0) {
		cerr << "queryList is Null" << endl;
		return 0;
	}

	struct timeval timeout;
	timeout.tv_sec = m_timeout/1000000;
	timeout.tv_usec = m_timeout%1000000;

	struct event_base *base = event_base_new();
	if(base == NULL) {
		cerr << "Couldn't open event base" << endl;
		return -1;
	}
	//加入超时定时器
	struct event *evtimeout = evtimer_new(base, timeoutcb, base);
	evtimer_add(evtimeout, &timeout);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = m_realAddr;
	sin.sin_port = htons(m_port);

	struct bufferevent **bevs = (struct bufferevent **)malloc(queryList.size() * sizeof(struct bufferevent *));

	EventRespon **eventRespons = (EventRespon **)malloc(queryList.size() * sizeof(EventRespon *));

	int remainRespon = queryList.size();
	int step = 0;
	for(size_t i = 0; i < queryList.size(); ++i) {
		struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		EventRespon *eventRespon = new EventRespon(queryList[i], &remainRespon, base, m_buffSize);
		eventRespons[i] = eventRespon;
		step ++;

		bufferevent_setcb(bev, readcb, NULL, eventcb, eventRespon);
		bufferevent_enable(bev, EV_READ|EV_WRITE);

		if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			cerr << "error connect" << endl;
			break;
		}
		bevs[i] = bev;
	}

	event_base_dispatch(base);
	for(int i = 0; i < step; ++i) {
		string ret;
		respon.push_back(ret);
		int pos = recvResult(eventRespons[i]->m_output);
		if(pos < 0) {
			cerr << "[Return ERROR]" << endl;
			continue;
		}
		decodeBody(respon[i], eventRespons[i]->m_output, pos);
	}
	for(int i = 0; i < step; ++i) {
		bufferevent_free(bevs[i]);
		delete eventRespons[i];
	}
	free(eventRespons);
	free(bevs);
	event_free(evtimeout);
	event_base_free(base);
	return 0;
}


void SocketClientAsyn::decodeBodyChunked(std::string& ret, char* buf) {
	char* p = buf;
	char t;
	int len = strlen(buf);
	while ((p-buf)<len){
		//获取chunk长度
		char* stop;
		int sz = strtol(p,&stop,16);
		if(!sz) {
			break;
		}
		//添加body内容
		p = stop+2;
		t = *(p+sz);
		*(p+sz) = '\0';
		ret += p;
		*(p+sz) = t;
		p = p+sz+2;
	}
	return;
}

void SocketClientAsyn::decodeBody(std::string& ret, char* buf, int body_pos) {
	ret = "";
	if(strstr(buf, "Transfer-Encoding: chunked") == NULL) {
		ret = buf+body_pos;
	}
	else {
		decodeBodyChunked(ret,buf+body_pos);
	}
	return;
}

//返回报文正文的起始位置
int SocketClientAsyn::recvResult(char* result) {
	int ret;
	char* head_ptr = strstr(result,"\r\n\r\n");
	if(head_ptr == NULL) {
		return 0;
	}
	else {
		*head_ptr = '\0';
		ret = (head_ptr-result)+4;
		return ret;
	}
}

void SocketClientAsyn::set_tcp_no_delay(evutil_socket_t fd) {
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}
void SocketClientAsyn::timeoutcb(evutil_socket_t fd, short what, void *arg) {
	struct event_base *base = (struct event_base *)arg;
	event_base_loopexit(base, NULL);
}
void SocketClientAsyn::readcb(struct bufferevent *bev, void *ctx) {
	struct evbuffer *input = bufferevent_get_input(bev);
	EventRespon *eventRespon = (EventRespon*)(ctx);
	int n, len = 0;
	while((n = evbuffer_remove(input, eventRespon->m_output, sizeof(char)*eventRespon->m_outputBuffSize)) > 0) {
		len += n;
	}
	(eventRespon->m_output)[len] = '\0';
	(*((int*)eventRespon->m_remainRespon)) --;
	if((*((int*)eventRespon->m_remainRespon)) == 0) {
		event_base_loopexit(eventRespon->m_base, NULL);
	}
}
void SocketClientAsyn::eventcb(struct bufferevent *bev, short events, void *ptr) {
	if(events & BEV_EVENT_CONNECTED) {
		evutil_socket_t fd = bufferevent_getfd(bev);
		set_tcp_no_delay(fd);
		// struct evbuffer *input = bufferevent_get_input(bev);
		EventRespon *eventRespon = (EventRespon*)(ptr);

		string query = eventRespon->m_input;
		string use_host = "default";
		string sendmsg_str = (string)"GET /" + query + " HTTP/1.1\r\n"+
		(string)"Accept: */*\r\n" +
		(string)"Accept-Language: zh-cn\r\n" +
		(string)"Host: " + use_host + "\r\n"+
		(string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n";
		bufferevent_write(bev, sendmsg_str.c_str(), sendmsg_str.size());
	}
	else if(events & BEV_EVENT_ERROR) {
		cerr << "NOT Connected" << endl;
	}
}

/*
int main() {
	SocketClientAsyn client;
	client.Init("127.0.0.1:8899", 10000000);
	vector<string> queryList, resp;
	string query = "?lang=zh_cn&ccy=CNY&uid=56hyukpt57de04e9df798273id10c2em&query={\"budget\":1971,\"budgetPredict\":6901,\"budgetReal\":-1,\"checkin\":\"20170129\",\"checkout\":\"20170202\",\"cid\":\"10002\",\"prefer\":{\"flight\":{\"class\":[],\"com\":[],\"type\":[]},\"global\":{\"baoche\":0,\"mode\":[\"flight\",\"train\"],\"special\":0,\"time\":[],\"transit\":1},\"hotel\":{\"addition\":[],\"facilities\":[],\"position\":\"3\",\"type\":[14]},\"train\":{\"class\":[]}}}&tid=jh5vo0pt57e7cbffdf75a462id12enbz&qid=1481617583900&type=csv010";
	queryList.push_back(query);
	queryList.push_back(query);
	client.GetRstFromHost(queryList, resp);
	for(int i = 0; i < resp.size(); i ++) {
		cerr << resp[i] << endl;
	}
	return 0;
}*/
