#pragma once
#include "function.h"

//�û���Ϣ��ָ��
class Task
{
public:
	Task(int fd, string orders, string username, int Dir)
		:fd(fd), orders(orders), username(username), Dir(Dir)
	{
	}
	~Task() {};

	//����Ϊ����
	int fd;				//�ͻ���socket
	string orders;		//�ͻ��˷�����������
	string username;	//�ͻ��˵�¼�û���
	int Dir;			//�û�����Ŀ¼��

private:

};

//�������
class WorkQue
{
public:
	WorkQue()
	{
		mutex = PTHREAD_MUTEX_INITIALIZER;		//???
	}
	~WorkQue() {};

	//����������в����µ�����
	void insertTask(const Task& task)
	{
		deq.push_back(task);
	}

	int size()
	{
		return deq.size();
	}

	//�߳�ȡ���µ�����
	Task getTask()
	{
		Task ans = deq.front();
		deq.pop_front();
		return ans;
	}

	//����Ϊ����
	pthread_mutex_t mutex;		//���߳�������еĻ�����
	deque<Task> deq;			//���accept���յ�fd

private:

};