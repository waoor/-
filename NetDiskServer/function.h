#pragma once
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <string.h>		//������string��������ʹ��memset
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


/*TCPͨ�Žṹ�嶨�ڲ���*/
#define IP "192.168.31.62"
#define PROT 2345
#define THREADNUM 10
#define MAXEVENTS 10


/**
 * ʹ��C++Ԥ��������ʹ����
 * '\'Ϊ�����ַ������ڽ��궨�������С�
 * perrorʱһ��C�⺯�������ڽ����һ��ϵͳ���õĴ����������׼����stderr������
 * @param ret ��������ֵ���ļ���������
 * @param retval ��׼ֵ
 * @param filename ָ���������ص��ļ���
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
	int dataLen;	//��ͷ��������¼�����ݵĴ�С
	char buf[1000];	//�����ݣ������������
};


//����TCPͨ��socket
int tcpInit(int*);

//���գ������ļ�
int recvFile(int sockfd);
int sendFile(int sockfd, const char*);

//ѭ������/��������
int recvCycle(int fd, void* p, size_t len);
int sendCycle(int fd, void* p, size_t len);

//��������ַ��������ڲ���Salt��
string GenerateStr(int STR_LEN);

//��ѯ����
void* ask(void* arg);