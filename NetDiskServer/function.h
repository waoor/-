#pragma once
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <string.h>		//不能用string，否则不能使用memset
#include <sys/wait.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <deque>
#include <vector>

#include <sstream>
#include <algorithm>

using namespace std;


/*TCP通信结构体定于部分*/
#define IP "192.168.31.62"
#define PROT 2345
#define THREADNUM 10
#define MAXEVENTS 10


/**
 * 使用C++预处理器宏和代码块
 * '\'为行续字符，用于将宏定义跨足多行。
 * perror时一个C库函数，用于将最近一次系统调用的错误输出到标准错误（stderr）流。
 * @param ret 函数返回值（文件描述符）
 * @param retval 标准值
 * @param filename 指定与错误相关的文件名
 */
#define ERROR_CHECK(ret,retval,filename) \
	{									 \
		if (ret == retval)				 \
		{								 \
			perror(filename);			 \
			return -1;					 \
		}								 \
	}	

struct Packet {
	int dataLen;	//包头，用来记录存数据的大小
	char buf[1000];	//包数据，用来存放数据
};


//创建TCP通信socket
int tcpInit(int*);

//接收，发送文件
int recvFile(int sockfd);
int sendFile(int sockfd, const char*);

//循环接收/发送数据
int recvCycle(int fd, void* p, size_t len);
int sendCycle(int fd, void* p, size_t len);

//产生随机字符串（用于产生Salt）
string GenerateStr(int STR_LEN);

//轮询函数
void* ask(void* arg);