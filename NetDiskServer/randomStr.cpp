#include "function.h"

//�������Salt
string GenerateStr(int STR_LEN)
{
	string str;
	int i, flag;
	srand(time(NULL));		//ͨ��ʱ�亯��������������ӣ�ʹ��ÿ�����н�����
	for (int i = 0; i < STR_LEN; ++i)
	{
		flag = rand() % 3;
		switch (flag)
		{
		case 0:
			str += rand() % 26 + 'a';
			break;
		case 1:
			str += rand() % 26 + 'A';
		case 2:
			str += rand() % 10 + '0';
			break;
		}
	}
	return str;
}