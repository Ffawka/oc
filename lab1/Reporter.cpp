#include <iostream>
#include <fstream>
#include <vector>
#include <conio.h>
#include <algorithm>

using namespace std;

struct employee
{
    int num;
    char name[10];
    double hours;
};

int main(int argc, char* argv[])
{

    string inputFile = argv[1];
    string outputFile = argv[2];
    double hourlyRate = atof(argv[3]);

    ifstream inFile(inputFile, ios::binary);
    if (!inFile)
    {
        cout << "Ошибка открытия файла: " << inputFile << endl;
        return 1;
    }

    vector <employee> employees;
    employee emp;

    while (inFile.read((char*)&emp, sizeof(employee)))
    {
        employees.push_back(emp);
    }

    sort(employees.begin(), employees.end(),
        [](const employee& a, const employee& b)
        { return a.num < b.num; });

    ofstream outFile(outputFile);
    if (!outFile)
    {
        cout << "Ошибка создания файла: " << outputFile << endl;
        return 1;
    }

    outFile << "Отчет по файлу: " << inputFile << endl;
    outFile << "ID\tИмя\tЧасы\tЗарплата" << endl;

    for (const employee& e : employees)
    {
        double salary = e.hours * hourlyRate;
        outFile << e.num << "\t"
            << e.name << "\t"
            << e.hours << "\t"
            << salary << endl;
    }

    outFile.close();
    cout << "Отчет " << outputFile << " создан успешно!" << endl;
    return 0;
}
