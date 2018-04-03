#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "MJLog.h"
#include <string>
#include "MyTimer.h"
#include "SocketClientEpoll.h"

using namespace MJ;

#define SEG_BUFF_SIZE 256

void SocketClientEpoll::setNonBlock(int fd) {   
    int flag = fcntl ( fd, F_GETFL, 0 );   
    fcntl ( fd, F_SETFL, flag | O_NONBLOCK );   
} 

int SocketClientEpoll::doHttpRequest(const std::string& addr, 
                             const int port,
                             const std::string& query,
                             char* respBuffer,
                             int& respBufferSize,
                             int timeout){
        int sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
        if(sk < 0) {   
            printf("[sock:%d]open socket failed!",sk);   
            return -1;   
        }
        struct sockaddr_in sa = {0};   
        sa.sin_family = AF_INET;   
        sa.sin_port = htons(port);   

        struct sockaddr_in *psa = &sa;   
        int iRet = inet_pton(AF_INET, addr.c_str(), &psa->sin_addr.s_addr); 
        if (iRet <= 0){
            printf("[sock:%d]Invalid Address:[%s]",sk,addr.c_str());
            close(sk);   
            return -1;
        }

        SocketClientEpoll::setNonBlock(sk);
        connect(sk, (struct sockaddr*)&sa, sizeof(sa));

          
        int efd;    
        efd = epoll_create(10);    
        if(efd == -1) {   
            printf("[sock:%d]epoll_create failed",sk);
            close(sk);
            return -1;
        }   
        struct epoll_event event;   
        struct epoll_event events[10];   
        event.events = EPOLLOUT | EPOLLIN | EPOLLET;   
        event.data.fd = sk;   
        epoll_ctl(efd, EPOLL_CTL_ADD, sk, &event);   

        int isOver = 0;   
        const char* qr = query.c_str();
        if (*qr == '/')
            qr++;
        std::string msg = (std::string)"GET /" + qr + " HTTP/1.1\r\n"+
                (std::string)"Accept: */*\r\n" +
                (std::string)"Accept-Language: zh-cn\r\n" +
                (std::string)"Host: " + "default" + "\r\n"+
                (std::string)"Content-Type: application/x-www-form-urlencoded;charset=utf-8\r\n\r\n";
        int dsize = msg.length(); 
        int wrest = dsize;
        int nsend;

        int seg = (SEG_BUFF_SIZE < respBufferSize) ? SEG_BUFF_SIZE : respBufferSize;
        memset(respBuffer, 0, respBufferSize); 
        int nread = 0;   
        int nrecv = 0;

        
        while(1){   
            MyTimer tmr;
            if(isOver > 0){   
                break;   
            }
            int n = epoll_wait(efd, events, 10, timeout); 
            if (n == 0){
                printf("[sock:%d]请求超时\n",sk);
                break;
            }  
            for(int i = 0; i < n; i++){   
                if(events[i].events & EPOLLOUT){   
                    printf("[sock:%d]EPOLLOUT...\n",sk);   
                    while(wrest > 0){   
                        nsend = write(events[i].data.fd, msg.c_str() + dsize - wrest, wrest);  
                        if(nsend < wrest){   
                            if (nsend < 0){
                                if (errno != EAGAIN){
                                    printf("[sock:%d]socket write failed\n",sk);   
                                    epoll_ctl(efd,EPOLL_CTL_DEL,sk,NULL);
                                    close(events[i].data.fd); 
                                    close(efd);   
                                    return -1;   
                                }else{
                                    printf("[sock:%d]waring:缓冲区写满\n",sk);
                                }
                                break;
                            }
                            printf("[sock:%d]write不完全(要发送%d,已发送%d)\n",sk,wrest,nsend);
                        }
                        wrest -= nsend; 
                    }
                    if (wrest == 0){
                        event.events = EPOLLIN | EPOLLET;
                        epoll_ctl(efd,EPOLL_CTL_MOD,sk,&event);
                    }   
                }   
                if(events[i].events & EPOLLIN) {   
                    printf("[sock:%d]EPOLLIN...\n",sk);   
                    while((nrecv = read(events[i].data.fd, respBuffer + nread, seg)) > 0){
                        nread += nrecv;
                        if (nread + seg + 1 > respBufferSize){
                            printf("[sock:%d]返回数据过大，请增加buffer\n",sk);
                            epoll_ctl(efd,EPOLL_CTL_DEL,sk,NULL);
                            close(events[i].data.fd);   
                            close(efd); 
                            return -1;
                        }
                    }   
                    if(nrecv < 0){   
                        if (errno != EAGAIN){
                            printf("[sock:%d]read failed\n",sk); 
                            epoll_ctl(efd,EPOLL_CTL_DEL,sk,NULL);  
                            close(events[i].data.fd);   
                            close(efd); 
                            return -1;   
                        }else{
                            printf("[sock:%d]waring:缓冲区读空\n",sk);
                        }
                    }else{
                        isOver += 1;    //下次循环退出
                        respBufferSize = nread;
                        printf("[sock:%d]RESPONSE:\n%s\n", sk, respBuffer);
                    }       
                }   
            } 
            if (timeout < 0)
                continue;
            timeout -= int(tmr.cost()/1000); 
            if (timeout < 0)
                timeout = 0;
        }  
        epoll_ctl(efd,EPOLL_CTL_DEL,sk,NULL);
        close(sk);   
        close(efd);   
        return 0;   
    }