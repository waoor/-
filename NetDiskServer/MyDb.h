#pragma once
#include <string>
#include <iostream>
#include <mysql/mysql.h>

using namespace std;

class MyDb {
public:
	MyDb();
	~MyDb();

	bool initDB(string host, string user, string passwd, string db_name, int port);
	bool exeSQL(string sql);
	bool select_one_SQL(string sql, string& str);
	bool select_many_SQL(string sql, string& str);
	
private:
	MYSQL* mysql;
	MYSQL_RES* result;
	MYSQL_ROW row;
};

MyDb::MyDb()
{
	mysql = mysql_init(NULL);
}

MyDb::~MyDb()
{
	if (mysql)
	{
		mysql_close(mysql);
	}
}

bool MyDb::initDB(string host, string user, string passwd, string db_name, int port = 3306)
{
	mysql = mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db_name.c_str(), port, NULL, 0);
	if (!mysql)
	{
		cout << "MySQL Error: " << mysql_error(mysql);
		return false;
	}
	return true;
}

bool MyDb::exeSQL(string sql)
{
	//ִ�гɹ�����0��ʧ�ܷ��ط�0ֵ
	if (mysql_query(mysql, sql.c_str()))
	{
		cout << "Query Error:" << mysql_error(mysql);
		return false;
	}

	result = mysql_store_result(mysql);

	if (result)
	{
		//��ȡ��������ܹ����ֶ�����������
		int num_fields = mysql_num_fields(result);

		unsigned long long num_rows = mysql_num_rows(result);

		for (unsigned long long i = 0; i < num_rows; ++i)
		{
			row = mysql_fetch_row(result);
			if (!row)
			{
				break;
			}
			
			for (int j = 0; j < num_fields; ++j)
			{
				cout << row[j] << "\t\t";
			}
			cout << endl;
		}
	}
	else
	{
		//ִ��update��insert��delete��ķǲ�ѯ���
		if (mysql_field_count(mysql) == 0)
		{
			//����update��insert��deleteӰ�������
			unsigned long long num_rows = mysql_affected_rows(mysql);

			return num_rows;
		}
		else
		{
			cout << "Get result error: " << mysql_error(mysql);
			return false;
		}
	}
	return true;
}

//ֻ����һ���ֶ�һ�У�������str�У���������ڣ�strΪ��
bool MyDb::select_one_SQL(string sql, string& str)
{
	//ִ�гɹ�����0��ʧ�ܷ��ط�0ֵ
	if (mysql_query(mysql, sql.c_str()))
	{
		cout << "Query Error:" << mysql_error(mysql);
		return false;
	}

	result = mysql_store_result(mysql);

	if (result)
	{
		//��ȡ��������ܹ����ֶ�����������
		int num_fields = mysql_num_fields(result);

		unsigned long long num_rows = mysql_num_rows(result);

		for (unsigned long long i = 0; i < num_rows; ++i)
		{
			row = mysql_fetch_row(result);
			if (!row)
			{
				break;
			}

			for (int j = 0; j < num_fields; ++j)
			{
				str = row[j];
			}
		}
	}
	return true;
}

//���Ҷ���ֶζ���У�������str��
bool MyDb::select_many_SQL(string sql, string& str)
{
	//ִ�гɹ�����0��ʧ�ܷ��ط�0ֵ
	if (mysql_query(mysql, sql.c_str()))
	{
		cout << "Query Error:" << mysql_error(mysql);
		return false;
	}

	result = mysql_store_result(mysql);

	if (result)
	{
		//��ȡ��������ܹ����ֶ�����������
		int num_fields = mysql_num_fields(result);

		unsigned long long num_rows = mysql_num_rows(result);

		for (unsigned long long i = 0; i < num_rows; ++i)
		{
			row = mysql_fetch_row(result);
			if (!row)
			{
				break;
			}

			for (int j = 0; j < num_fields; ++j)
			{
				str += row[j];
				str += "|\t\t";
			}
			str += "\n";
		}
	}
	return true;
}