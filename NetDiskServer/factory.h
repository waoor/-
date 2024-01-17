#pragma once
#include "workQue.h"
#include "MyLog.h"


void* doingTask(void* arg);				//Ϊʲô��������Ϊ��Ա����
int childHandle(const Task& task);

class Factory
{
public:
	Factory()
	{
		threadNum = THREADNUM;
		cond = PTHREAD_COND_INITIALIZER;
		startFlag = false;
	}

	~Factory() {};

	void startFactory()
	{
		if (!startFlag)
		{
			cout << "server is running..." << endl;
			for (int i = 0; i < threadNum; ++i)
			{
				pthread_create(&threads[i], NULL, doingTask, this);
			}
			sleep(1);
			startFlag = true;
		}
	}

	//�˴�����Ϊ���г�Ա��doingTask�����ſ��Ե���que��Ϊʲô������
	WorkQue que;
	int threadNum;
	pthread_t threads[THREADNUM];
	pthread_cond_t cond;
	bool startFlag;

private:

};

//
void* doingTask(void* arg)
{
	Factory* f = (Factory*)arg;
	while (1)
	{
		pthread_mutex_lock(&f->que.mutex);
		while (0 == f->que.size())
		{
			pthread_cond_wait(&f->cond, &f->que.mutex);
		}
		Task task = f->que.getTask();
		childHandle(task);
		pthread_mutex_unlock(&f->que.mutex);
	}
}

//���߳�ֻ������puts��gets�������ȽϺ�ʱ������
int childHandle(const Task& task)
{
	MyDb db;
	db.initDB("localhost", "linuxsql", "123", "Netdisk");
	int dataLen;
	char buf[1000] = { 0 };
	int ret;
	bool flag;
	string sql;
	string res;
	off_t filesize;

	//���������������
	int sockfd = task.fd;
	stringstream ss(task.orders);
	cout << "ChildHandle:" << task.orders << endl;
	string order, name, order2;	//order���name����name������ļ�����order2���������Ϊ�գ�
	ss >> order >> name >> order2;
	string filename(name);
	const char* FILENAME = filename.c_str();

	string username = task.username;
	cout << "username:" << username << endl;

	int Dir = task.Dir;
	cout << "Dir:" << Dir << endl;
	
	LOG(username, task.orders);

	/**********************puts �ļ�����*********************/
	if ("puts" == order)
	{
		//����MD5��
		bzero(buf, sizeof(buf));
		recvCycle(sockfd, &dataLen, 4);
		recvCycle(sockfd, buf, dataLen);
		
		string md5(buf);
		cout << "md5:" << md5 << endl;

		/**
		* putsǰ��Ҫ���������жϣ�
		* 1.�ڵ�ǰ�û�����ǰĿ¼���Ƿ�ӵ�д��ļ�
		* 2.����Ŀ¼���Ѿ�������ͬ���ļ�������ֱ���봫
		*/

		//1.�ж��ļ��Ƿ��ظ�
		sql = "SELECT Filename FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'f' AND Filename = '" + filename + "'";
		db.select_one_SQL(sql, res);
		if (res.empty())
		{
			flag = true;
			sendCycle(sockfd, &flag, 1);
		}
		else
		{
			flag = false;
			sendCycle(sockfd, &flag, 1);
			return 0;
		}

		//2.�ж��Ƿ�ʹ���봫
		sql = "SELECT MD5 FROM Virtual_Dir WHERE MD5 = '" + md5 + "'";
		db.select_one_SQL(sql, res);
		cout << sql << endl;
		cout << res << endl;
		if (!res.empty())
		{
			cout << "second pass" << endl;
			flag = true;
			sendCycle(sockfd, &flag, 1);
		}
		else
		{
			flag = false;
			sendCycle(sockfd, &flag, 1);
			int file_fd = open(md5.c_str(), O_RDWR | O_CREAT, 0666);
			ERROR_CHECK(file_fd, -1, "open");

			//�����ļ���С
			bzero(buf, sizeof(buf));
			recvCycle(sockfd, &dataLen, sizeof(int));
			recvCycle(sockfd, buf, dataLen);
			memcpy(&filesize, buf, dataLen);
			ftruncate(file_fd, filesize);

			//�����ļ�����
			bzero(buf, sizeof(buf));
			char* pmap = (char*)mmap(NULL, filesize, PROT_WRITE, MAP_SHARED, file_fd, 0);
			ERROR_CHECK(pmap, (char*)-1, "mmap");
			off_t download = 0, lastsize = 0;
			off_t slice = filesize / 100;
			while (1)
			{
				recvCycle(sockfd, &dataLen, sizeof(int));
				if (0 == dataLen)
				{
					printf("\r100.00%%\n");
					break;
				}
				else
				{
					ret = recvCycle(sockfd, pmap + download, dataLen);
					if (ret == 0)
					{
						break;	//�������ѶϿ�
					}
					download += dataLen;
					if (download - lastsize > slice)
					{
						printf("\r%5.2f%%", (float)download / filesize * 100);
						fflush(stdout);
						lastsize = download;
					}
				}
			}
			ret = munmap(pmap, filesize);
			ERROR_CHECK(ret, -1, "munmap");
			close(file_fd);
		}
		sql = "INSERT INTO Virtual_Dir(Dir,FileName,FileType,MD5,User) VALUES(" + to_string(Dir) + ",'" + filename + "','f','" + md5 + "','" + username + "')";
		db.exeSQL(sql);
	}
	/**********************gets �ļ�����*********************/
	else if ("gets" == order)
	{
		//���жϵ�ǰĿ¼�����޸��ļ�
		sql = "SELECT FileName FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'f' AND FileName = '" + filename + "'";
		db.select_one_SQL(sql, res);
		if (!res.empty())
		{
			flag = true;
			sendCycle(sockfd, &flag, 1);
		}
		else
		{
			flag = false;
			sendCycle(sockfd, &flag, 1);
			return 0;
		}

		//�������ļ�Ŀ¼��ʵ���ļ����ƵĴ洢��ʹ��MD5����Ϊ���ֵ�
		sql = "SELECT MD5 FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'f' AND FileName = '" + filename + "'";
		db.select_one_SQL(sql, res);
		FILENAME = res.c_str();

		//�ж��ļ���ƫ�ƣ��ϵ��������ܣ�
		off_t beginPos = stol(order2);
		cout << "beginPos = " << beginPos << endl;

		//��ʼ�����ļ�
		Packet packet;
		int file_fd = open(FILENAME, O_RDWR);
		ERROR_CHECK(file_fd, -1, "open");

		//�����ļ���С
		struct stat statbuf;
		ret = stat(FILENAME, &statbuf);
		if (ret == -1)
		{
			perror("stat");
			pthread_exit(NULL);
		}
		off_t filesize = statbuf.st_size;
		packet.dataLen = sizeof(off_t);
		memcpy(packet.buf, &filesize, packet.dataLen);
		sendCycle(sockfd, &packet, sizeof(int) + packet.dataLen);

		//�����ļ�����
		char* pmap = (char*)mmap(NULL, filesize, PROT_READ, MAP_SHARED, file_fd, 0);
		ERROR_CHECK(pmap, (char*)-1, "mmap");
		off_t offset = beginPos, lastsize = beginPos;
		off_t slice = filesize / 100;
		while (1)
		{
			if (filesize > offset + (off_t)sizeof(packet.buf))
			{
				packet.dataLen = sizeof(packet.buf);
				memcpy(packet.buf, pmap + offset, packet.dataLen);
				ret = sendCycle(sockfd, &packet, sizeof(int) + packet.dataLen);
				if (ret == -1)
				{
					close(file_fd);
					return -1;
				}
				offset += packet.dataLen;
				//��ӡ
				if (offset - lastsize > slice)
				{
					printf("\r%5.2f%%", (float)offset / filesize * 100);
					fflush(stdout);
					lastsize = offset;
				}
			}
			else
			{
				packet.dataLen = filesize - offset;
				memcpy(packet.buf, pmap + offset, packet.dataLen);
				ret = sendCycle(sockfd, &packet, sizeof(int) + packet.dataLen);
				if (ret == -1)
				{
					close(file_fd);
					return -1;
				}
				break;
			}
		}
		printf("\r100.0000000%%\n");
		ret = munmap(pmap, filesize);
		ERROR_CHECK(ret, -1, "munmap");

		//���ʹ��ͽ�����־
		packet.dataLen = 0;
		sendCycle(sockfd, &packet, sizeof(int));
		close(file_fd);
	}
	return 0;
}