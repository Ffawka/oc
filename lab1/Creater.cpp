#include <iostream>
#include <fstream>
#include <vector>
#include <conio.h>

using namespace std;

struct employee
{
	int num;
	char name[10];
	double hours;
};

int main(int argc, char* argv[])
{
	system("chcp 1251 > NULL");
	string filename = argv[1];
	int recordCount = atoi(argv[2]);

	ofstream outFile(filename, ios::binary);
	if (!outFile)
	{
		cout << "Ошибка при создании файла" << endl;
		return 1;
	}

	cout << "Количество записей = " << recordCount << endl;

	for (int i = 0; i < recordCount; i++)
	{
		employee emp;

		cout << "Запись = " << i + 1 << endl;
		cout << "Номер сотрудника = ";
		cin >> emp.num;
		cout << "Имя сотрудника = ";
		cin >> emp.name;
		cout << "Количество отработанных часов = ";
		cin >> emp.hours;

		outFile.write((char*)&emp, sizeof(employee));
	}

	cout << "Файл создан" << endl;
	return 0;
}
