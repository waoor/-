#pragma once
#include "function.h"

//用户信息及指令
class Task
{
public:
	Task(int fd, string orders, string username, int Dir)
		:fd(fd), orders(orders), username(username), Dir(Dir)
	{
	}
	~Task() {};

	//声明为公有
	int fd;				//客户端socket
	string orders;		//客户端发送来的命令
	string username;	//客户端登录用户名
	int Dir;			//用户所在目录号

private:

};

//任务队列
class WorkQue
{
public:
	WorkQue()
	{
		mutex = PTHREAD_MUTEX_INITIALIZER;		//???
	}
	~WorkQue() {};

	//向任务队列中插入新的任务
	void insertTask(const Task& task)
	{
		deq.push_back(task);
	}

	int size()
	{
		return deq.size();
	}

	//线程取得新的任务
	Task getTask()
	{
		Task ans = deq.front();
		deq.pop_front();
		return ans;
	}

	//声明为公有
	pthread_mutex_t mutex;		//多线程任务队列的互斥锁
	deque<Task> deq;			//存放accept接收的fd

private:

};