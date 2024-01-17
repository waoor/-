#pragma once
#include "function.h"
#include <unordered_map>

class Timer
{
public:
	Timer()
	{
		//�Ӵ����Ϳ�ʼ��ѯ
		pthread_t pth;
		pthread_create(&pth, NULL, ask, &timeMap);
		sleep(1);
	}
	~Timer() {};

	//���µĿͻ���������
	void add(const int& fd)
	{
		time_t t;
		time(&t);
		timeMap[fd] = t;
	}

	//�ͻ��˹ر�
	void deleteFd(const int& fd)
	{
		timeMap.erase(fd);
	}

	//�ͻ��˷�������
	void update(const int& fd)
	{
		time_t t;
		time(&t);
		timeMap[fd] = t;
	}

private:
	unordered_map<int, time_t> timeMap;	//<�û���fd,��һ�η��Ͱ���ʱ��>
};
