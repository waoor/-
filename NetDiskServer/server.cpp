#include "function.h"
#include "factory.h"
#include "timer.h"

Timer myTimer;							//轮询扫描
vector<int> clients;					//存储客户端的描述符
unordered_map<string, int> name_to_fd;	//<用户名称，描述符>
int epfd = epoll_create(1);				//设置epoll


//轮询函数
void* ask(void* arg)
{
	unordered_map<int, time_t>* timeMap = (unordered_map<int, time_t>*)arg;	//<用户fd，上一次发送包的时间>
	while (1)
	{
		for (auto& e : *timeMap)
		{
			time_t t;
			time(&t);
			if (t - e.second > 10000)
			{
				cout << "Client " << e.first << " timeout has not sent a message, disconnected!" << endl;
				//取消监听并从clients，timeMap中删除
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
	/*************启动多线程，并绑定TCP的socket***********/
	Factory f;
	Packet packet;
	f.startFactory();			//启动工厂
	int sockfd;
	tcpInit(&sockfd);

	bool flag;

	/*************设置epoll，监听sockfd与客户端描述符***********/
	ERROR_CHECK(epfd, -1, "epoll_create");
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	ERROR_CHECK(ret, -1, "epoll_ctl");

	/*****************启动服务器，并连接服务器数据库***************/
	chdir("../workspace/");		//???

	MyDb db;
	db.initDB("localhost", "linuxsql", "123", "Netdisk");
	string sql;					//存储数据库命令
	int dataLen = 0;			//数据长度
	char buf[1000] = { 0 };		//缓存器
	
	while (1)
	{
		struct epoll_event evs[MAXEVENTS];		//监听sockfd，以及所有accept的newfd
		int nfds = epoll_wait(epfd, evs, MAXEVENTS, 10000);
		ERROR_CHECK(nfds, -1, "epoll_wait");
		
		for (int i = 0; i < nfds; ++i)
		{
			/*************有新的客户端连接***********/
			/**
			 *接收新连接的描述符信息：
			 * logIn 登录
			 * signIn 注册
			 * puts 上传操作
			 * gets 下载操作
			 * quit 退出
			 */
			if (evs[i].data.fd == sockfd)	//监听的fd收到消息，有客户端进行连接
			{
				int newClient = accept(sockfd, NULL, NULL);		//accept()，为什么参数为空？？？
				ERROR_CHECK(newClient, -1, "accept");
				
				//服务器接收到客户端发来的命令
			accept_client:		//???
				bzero(buf, sizeof(buf));
				recvCycle(newClient, &dataLen, 4);
				recvCycle(newClient, buf, dataLen);

				string orders(buf);
				cout << "receive orders From newClient:" << orders << endl;
				stringstream ss(orders);
				string order, name, order2;	//order表示命令，name表示跟在name后面的文件名，order2表示补充命令（可为空）
				ss >> order >> name >> order2;

				/*************登录操作***********/
				if (order == "logIn")
				{
					string salt;
					sql = "SELECT Salt FROM Shadow WHERE User = '" + name + "'";
					cout << sql << endl;
					//搜索此用户
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

					//若有此用户，将用户的盐值发送给客户端
					packet.dataLen = strlen(salt.c_str());
					memcpy(packet.buf, salt.c_str(), packet.dataLen);
					
					sendCycle(newClient, &packet, 4 + packet.dataLen);		//？？？

					//接收用户加密后的密文cipher
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &dataLen, 4);
					recvCycle(newClient, buf, dataLen);

					//进行最终的对比，判断密码是否正确
					string s(buf);			
					string cipher;			//密码
					sql = "SELECT Cipher FROM Shadow WHERE User = '" + name + "'";
					cout << sql << endl;
					db.select_one_SQL(sql, cipher);
					//输出slat后的密码
					cout << name << ":" << "s=" << s << ";" << "cipher=" << cipher << endl;

					if (s == cipher)
					{
						//密码验证成功，将描述符加入监听
						flag = true;
						ev.data.fd = newClient;
						ret = epoll_ctl(epfd, EPOLL_CTL_ADD, newClient, &ev);	//监听新连接上的客户端
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
						//密码验证失败
						flag = false;
						sendCycle(newClient, &flag, 1);
						goto accept_client;
					}
					LOG(name, orders);
				}
				/*************注册操作***********/
				else if (order == "signIn")
				{
					//在数据库中寻找是否有相同的用户
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
				/*************令牌操作，用于频繁请求数据的puts和gets***********/
				else if (order == "puts")
				{
					//接收用户姓名
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &dataLen, 4);
					recvCycle(newClient, buf, dataLen);
					string username(buf);

					//接收目录号
					int Dir;
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &Dir, 4);
					myTimer.update(name_to_fd[username]);

					//向任务队列插入puts任务
					cout << "upLoad thread is running ..." << endl;
					pthread_mutex_lock(&f.que.mutex);
					Task task(newClient, orders, username, Dir);
					f.que.insertTask(task);
					pthread_mutex_unlock(&f.que.mutex);
					pthread_cond_signal(&f.cond);
				}
				else if (order == "gets")
				{
					//接收用户姓名
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &dataLen, 4);
					recvCycle(newClient, buf, dataLen);
					string username(buf);

					//接收目录号
					int Dir;
					bzero(buf, sizeof(buf));
					recvCycle(newClient, &Dir, 4);
					myTimer.update(name_to_fd[username]);

					//向任务队列插入gets任务
					cout << "downLoad thread is running ..." << endl;
					pthread_mutex_lock(&f.que.mutex);
					Task task(newClient, orders, username, Dir);
					f.que.insertTask(task);
					pthread_mutex_unlock(&f.que.mutex);
					pthread_cond_signal(&f.cond);

				}
				/*************退出操作***********/
				else if (order == "quit")
				{
					cout << "client quit" << endl;
				}

			}
		}

		/*****************接收客户端发送的命令***************/
		for (size_t i = 0; i < clients.size(); ++i)
		{
			if (evs[i].data.fd == clients[i])	//客户端发来请求，主线程进行处理
			{
				//服务器接收到客户端发来的命令
				bzero(buf, sizeof(buf));
				recvCycle(clients[i], &dataLen, 4);
				ret = recvCycle(clients[i], buf, dataLen);
				if (ret == 0)
				{
					//接收到0，说明客户端已断开
					ev.data.fd = clients[i];
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clients[i], &ev);
					ERROR_CHECK(ret, -1, "epoll_ctl");
					clients.erase(remove(clients.begin(), clients.end(), clients[i]), clients.end());
				}
				cout << "Main thread process orders:" << buf << endl;

				string orders(buf);
				stringstream ss(orders);
				string order, name, order2;		//order表示命令，name表示跟在name后面的文件名，order2表示补充命令（可为空）
				ss >> order >> name >> order2;

				/*************ls***********/
				if (order == "ls")
				{
					//接收用户名
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//接收目录号
					int Dir;
					recvCycle(clients[i], &Dir, 4);

					//读取虚拟文件目录并返回
					sql = "SELECT FileNmae, FileType FROM Virtual_Dir WHERE User = '" + username + "' AND DIR = " + to_string(Dir);
					string res;
					db.select_many_SQL(sql, res);

					//发送消息
					packet.dataLen = strlen(res.c_str());
					memcpy(packet.buf, res.c_str(), packet.dataLen);
					sendCycle(clients[i], &packet, 4 + packet.dataLen);
				}
				/*************mkdir***********/
				else if (order == "mkdir")
				{
					//接收用户名
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], buf, dataLen);
					string username(buf);
					LOG(username, orders);
					
					//接收目录号
					int Dir;
					recvCycle(clients[i], &Dir, 4);
					sql = "SELECT FileName FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'd' AND FileName = '" + name + "'";

					string res;
					db.select_one_SQL(sql, res);
					if (res.empty())
					{
						flag = true;
						//创建目录
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
					//接收用户名
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], &buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//接收目录号
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
					//接收用户名
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], &buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//接收目录号
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
					//接收用户名
					bzero(buf, sizeof(buf));
					recvCycle(clients[i], &dataLen, 4);
					recvCycle(clients[i], buf, dataLen);
					string username(buf);
					LOG(username, orders);

					//接收目录号
					int Dir;
					recvCycle(clients[i], &Dir, 4);
					sql = "SELECT FileName FROM Virtual_Dir WHERE User = '" + username + "' AND Dir = " + to_string(Dir) + " AND FileType = 'f' AND FileName = '" + name + "'";

					string res;
					db.select_one_SQL(sql, res);
					if (!res.empty())
					{
						flag = true;
						//删除成功
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