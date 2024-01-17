#include "function.h"

int tcpInit(int* fd)
{
	int ret;
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	ERROR_CHECK(sockfd, -1, "socket");
	
	//�����׽��ֿ�ѡ��
	//SO_REUSEADDRĬ��ֵΪ0���٣�����Ϊ1�ɽ�Time-wait״̬�µ��׽��ֶ˿����·�����µ��׽���
	int on = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	ERROR_CHECK(ret, -1, "setsocket");

	//�����ַ��ʼ��
	struct sockaddr_in sockinfo;
	memset(&sockinfo, 0, sizeof(sockinfo));
	sockinfo.sin_addr.s_addr = inet_addr(IP);
	sockinfo.sin_family = AF_INET;
	sockinfo.sin_port = htons(PROT);
	
	ret = bind(sockfd, (sockaddr*)&sockinfo, sizeof(sockinfo));
	ERROR_CHECK(ret, -1, "bind");

	ret = listen(sockfd, 10);
	ERROR_CHECK(ret, -1, "listen");

	*fd = sockfd;
	return 0;
}