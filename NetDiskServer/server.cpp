#include "function.h"
#include "factory.h"
#include "timer.h"

Timer myTimer;							//��ѯɨ��
vector<int> clients;					//�洢�ͻ��˵�������
unordered_map<string, int> name_to_fd;	//<�û����ƣ�������>
int epfd = epoll_create(1);				//����epoll


//��ѯ����
void* ask(void* arg)
{
	unordered_map<int, time_t>* timeMap = (unordered_map<int, time_t>*)arg;	//<�û�fd����һ�η��Ͱ���ʱ��>
	while (1)
	{
		for (auto& e : *timeMap)
		{
			time_t t;
			time(&t);
			if (t - e.second > 10000)
			{
				cout << "Client " << e.first << " timeout has not sent a message, disconnected!" << endl;
				//ȡ����������clients��timeMap��ɾ��
				struct epoll_event ev;
				ev.data.fd = e.first;
				ev.events = EPOLLIN;

				int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, e.first, &ev);
				if (ret == -1)
				{
					perror("epoll_ctl");
					pthread_exit(NULL);
				}
				clients.erase(remove(clients.begin(), clients.end(), e.first), clients.end());
				timeMap->erase(e.first);
				close(e.first);
			}
			sleep(1);
		}
	}
}

int main()
{
	/*************�������̣߳�����TCP��socket***********/
	Factory f;
	Packet packet;
	f.startFactory();			//��������
	int sockfd;
	tcpInit(&sockfd);

	bool flag;

	/*************����epoll������sockfd��ͻ���������***********/
	ERROR_CHECK(epfd, -1, "epoll_create");
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	ERROR_CHECK(ret, -1, "epoll_ctl");

	/*****************�����������������ӷ��������ݿ�***************/
	chdir("../workspace/");		//???

	MyDb db;
	db.initDB("localhost", "linuxsql", "123", "Netdisk");
	string sql;					//�洢���ݿ�����
	int dataLen = 0;			//���ݳ���
	char buf[1000] = { 0 };		//������
	
	while (1)
	{
		struct epoll_event evs[MAXEVENTS];		//����sockfd���Լ�����accept��newfd
		int nfds = epoll_wait(epfd, evs, MAXEVENTS, 10000);
		ERROR_CHECK(nfds, -1, "epoll_wait");
		
		for (int i = 0; i < nfds; ++i)
		{
			/*************���µĿͻ�������***********/
			/**
			 *���������ӵ���������Ϣ��
			 * logIn ��¼
			 * signIn ע��
			 * puts �ϴ�����
			 * gets ���ز���
			 * quit �˳�
			 */
			if (evs[i].data.fd == sockfd)	//������fd�յ���Ϣ���пͻ��˽�������
			{
				int newClient = accept(sockfd, NULL, NULL);		//accept()��Ϊʲô����Ϊ�գ�����
				ERROR_CHECK(newClient, -1, "accept");
				
				//���������յ��ͻ��˷���������
			accept_client:		//???
				bzero(buf, sizeof(buf));
				recvCycle(newClient, &dataLen, 4);
				recvCycle(newClient, buf, dataLen);

				string orders(buf);
				cout << "receive orders From newClient:" << orders << endl;
				stringstream ss(orders);
				string order, name, order2;	//order��ʾ���name��ʾ����name������ļ�����order2��ʾ���������Ϊ�գ�
				ss >> order >> name >> order2;

				/*************��¼����***********/
				if (order == "logIn")
				{
					string salt;
					sql = "SELECT Salt FROM Shadow WHERE User = '" + name + "'";
					cout << sql << endl;
					//�������û�
					db.select_one_SQL(sql, salt);
					if (sql.empty())
					{
						flag = false;
						sendCycle(newClient, &flag, 1);
						goto accept_client;
					}
					else
					{
						flag = true;
						sendCycle(newClient, &flag, 1);
					}

					//���д��û������û�����ֵ���͸��ͻ���
					packet.dataLen = strlen(salt.c_str());
					memcpy(packet.buf, salt.c_str(), packet.dataLen);
					
					sendCycle(newClient, &packet, 4 + packet.dataLen);		//������

					//�����û����ܺ������cipher
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &dataLen, 4);
					recvCycle(newClient, buf, dataLen);

					//�������յĶԱȣ��ж������Ƿ���ȷ
					string s(buf);			
					string cipher;			//����
					sql = "SELECT Cipher FROM Shadow WHERE User = '" + name + "'";
					cout << sql << endl;
					db.select_one_SQL(sql, cipher);
					//���slat�������
					cout << name << ":" << "s=" << s << ";" << "cipher=" << cipher << endl;

					if (s == cipher)
					{
						//������֤�ɹ������������������
						flag = true;
						ev.data.fd = newClient;
						ret = epoll_ctl(epfd, EPOLL_CTL_ADD, newClient, &ev);	//�����������ϵĿͻ���
						ERROR_CHECK(ret, -1, "epoll_ctl");
						clients.push_back(newClient);
						sendCycle(newClient, &flag, 1);
						cout << newClient << " has connected!" << endl;
						name_to_fd[name] = newClient;
						cout << name << ":" << newClient << endl;
						myTimer.add(newClient);
					}
					else
					{
						//������֤ʧ��
						flag = false;
						sendCycle(newClient, &flag, 1);
						goto accept_client;
					}
					LOG(name, orders);
				}
				/*************ע�����***********/
				else if (order == "signIn")
				{
					//�����ݿ���Ѱ���Ƿ�����ͬ���û�
					string s;
					sql = "SELECT User FROM Shadow WHERE User = '" + name + "'";
					cout << sql << endl;
					db.select_one_SQL(sql, s);
					if (s.empty())
					{
						flag = true;
						string salt(GenerateStr(8));
						string cipher(crypt(order2.c_str(), salt.c_str()));
						sql = "INSERT INTO Shadow Values('" + name + "','" + order2 + "','" + salt + "','" + cipher + "')";
						cout << sql << endl;
						db.exeSQL(sql);
					}
					else
					{
						flag = false;
					}
					sendCycle(newClient, &flag, 1);
					LOG(name, orders);
					goto accept_client;
				}
				/*************���Ʋ���������Ƶ���������ݵ�puts��gets***********/
				else if (order == "puts")
				{
					//�����û�����
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &dataLen, 4);
					recvCycle(newClient, buf, dataLen);
					string username(buf);

					//����Ŀ¼��
					int Dir;
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &Dir, 4);
					myTimer.update(name_to_fd[username]);

					//��������в���puts����
					cout << "upLoad thread is running ..." << endl;
					pthread_mutex_lock(&f.que.mutex);
					Task task(newClient, orders, username, Dir);
					f.que.insertTask(task);
					pthread_mutex_unlock(&f.que.mutex);
					pthread_cond_signal(&f.cond);
				}
				else if (order == "gets")
				{
					//�����û�����
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &dataLen, 4);
					recvCycle(newClient, buf, dataLen);
					string username(buf);

					//����Ŀ¼��
					int Dir;
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &Dir, 4);
					myTimer.update(name_to_fd[username]);

					//��������в���gets����
					cout << "downLoad thread is running ..." << endl;
					pthread_mutex_lock(&f.que.mutex);
					Task task(newClient, orders, username, Dir);
					f.que.insertTask(task);
					pthread_mutex_unlock(&f.que.mutex);
					pthread_cond_signal(&f.cond);

				}
				/*************�˳�����***********/
				else if (order == "quit")
				{
					cout << "client quit" << endl;
				}

			}
		}

		/*****************���տͻ��˷��͵�����***************/
		for (size_t i = 0; i < clients.size(); ++i)
		{
			if (evs[i].data.fd == clients[i])	//�ͻ��˷����������߳̽��д���
			{
				//���������յ��ͻ��˷���������
				bzero(buf, sizeof(buf));
				recvCycle(clients[i], &dataLen, 4);
				ret = recvCycle(clients[i], buf, dataLen);
				if (ret == 0)
				{
					//���յ�0��˵���ͻ����ѶϿ�
					ev.data.fd = clients[i];
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clients[i], &ev);
					ERROR_CHECK(ret, -1, "epoll_ctl");
					clients.erase(remove(clients.begin(), clients.end(), clients[i]), clients.end());
				}
				cout << "Main thread process orders:" << buf << endl;

				string orders(buf);
				stringstream ss(orders);
				string order, name, order2;		//order��ʾ���name��ʾ����name������ļ�����order2��ʾ���������Ϊ�գ�
				ss >> order >> name >> order2;

				/*************ls***********/
				if (order == "ls")
				{
					//�����û���
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//����Ŀ¼��
					int Dir;
					recvCycle(clients[i], &Dir, 4);

					//��ȡ�����ļ�Ŀ¼������
					sql = "SELECT FileNmae, FileType FROM Virtual_Dir WHERE User = '" + username + "' AND DIR = " + to_string(Dir);
					string res;
					db.select_many_SQL(sql, res);

					//������Ϣ
					packet.dataLen = strlen(res.c_str());
					memcpy(packet.buf, res.c_str(), packet.dataLen);
					sendCycle(clients[i], &packet, 4 + packet.dataLen);
				}
				/*************mkdir***********/
				else if (order == "mkdir")
				{
					//�����û���
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], buf, dataLen);
					string username(buf);
					LOG(username, orders);
					
					//����Ŀ¼��
					int Dir;
					recvCycle(clients[i], &Dir, 4);
					sql = "SELECT FileName FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'd' AND FileName = '" + name + "'";

					string res;
					db.select_one_SQL(sql, res);
					if (res.empty())
					{
						flag = true;
						//����Ŀ¼
						sql = "INSERT INTO Virtual_Dir(Dir,FileName,FileType,User) VALUES(" + to_string(Dir) + ",'" + name + "','d','" + username + "')";
						db.exeSQL(sql);
					}
					else
					{
						flag = false;
					}
					sendCycle(clients[i], &flag, 1);
				}
				/*************rmkdir***********/
				else if (order == "rmdir")
				{
					//�����û���
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], &buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//����Ŀ¼��
					int Dir;
					recvCycle(clients[i], &Dir, 4);
					sql = "SELECT FileName FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileName = '" + name + "'";
					string res;
					db.select_one_SQL(sql, res);
					if (res.empty())
					{
						flag = false;
					}
					else
					{
						flag = true;
						sql = "DELETE FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileName = '" + name + "'";
						db.exeSQL(sql);
					}
					sendCycle(clients[i], &flag, 1);
				}
				/*************cd***********/
				else if (order == "cd")
				{
					//�����û���
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], &buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//����Ŀ¼��
					int Dir;
					recvCycle(clients[i], &Dir, 4);

					if (name == "..")
					{
						sql = "SELECT Dir FROM Virtual_Dir WHERE User = '" + username + "' AND FileId = " + to_string(Dir);
					}
					else
					{
						sql = "SELECT FileId FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileName = '" + name + "'";
					}

					string res;
					db.select_one_SQL(sql, res);
					if (res.empty())
					{
						flag = false;
						sendCycle(clients[i], &flag, 1);
					}
					else
					{
						flag = true;
						sendCycle(clients[i], &flag, 1);
						Dir = stoi(res);
						sendCycle(clients[i], &Dir, 4);
					}
					
				}
				/*************rm***********/
				else if (order == "rm")
				{
					//�����û���
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//����Ŀ¼��
					int Dir;
					recvCycle(clients[i], &Dir, 4);
					sql = "SELECT FileName FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'f' AND FileName = '" + name + "'";

					string res;
					db.select_one_SQL(sql, res);
					if (!res.empty())
					{
						flag = true;
						//ɾ���ɹ�
						sql = "DELETE FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'f' AND FileName = '" + name + "'";
						db.exeSQL(sql);
					}
					else
					{
						flag = false;
					}
					sendCycle(clients[i], &flag, 1);
				}
				else if (order == "quit")
				{
					ev.data.fd = clients[i];
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clients[i], &ev);
					ERROR_CHECK(ret, -1, "epoll_ctl");
					clients.erase(remove(clients.begin(), clients.end(), clients[i]), clients.end());
					myTimer.deleteFd(clients[i]);
					close(clients[i]);
				}
			}
			myTimer.update(clients[i]);
		}
	}
	close(sockfd);
	close(epfd);
	return 0;
}