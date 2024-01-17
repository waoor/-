#pragma once

#include "function.h"
#include "MyDb.h"

#define LOG(user,operation){MyLog log; log.insert(user, operation);}		//???

class MyLog
{
public:
	MyLog()
	{
		db.initDB("localhost", "linuxsql", "123", "Netdisk");
	}
	~MyLog() {};

	void insert(const string& user, const string& operation)
	{
		time_t t;
		time(&t);
		string stime(ctime(&t));
		string sql = "INSERT INTO Log (User, Operation, time) VALUES ('" + user + "','" + operation + "','" + stime + "')";
		db.exeSQL(sql);
	}

private:
	MyDb db;
};


