#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

struct employee
{
    int num;
    char name[10];
    double hours;
};

wstring stringToWstring(const string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

wstring findExeFile(const string& filename)
{
    vector<string> searchPaths =
    {
        "C:\\Users\\Fafka\\Desktop\\oc\\lab1\\Main\\x64\\Debug\\",
    };

    for (const string& path : searchPaths)
    {
        string fullPath = path + filename;
        ifstream test(fullPath.c_str());
        if (test.good())
        {
            test.close();
            cout << "Найден в " << fullPath << endl;
            return stringToWstring(fullPath);
        }
    }

    return L"";
}

void showBinaryFile(const string& filename)
{
    ifstream file(filename, ios::binary);
    if (!file)
    {
        cout << "Не удалось открыть файл " << filename << endl;
        return;
    }

    cout << "Содержимое бинарного файла:" << endl;
    cout << "ID\tИмя\tЧасы" << endl;

    employee emp;
    while (file.read((char*)&emp, sizeof(employee)))
    {
        cout << emp.num << "\t" << emp.name << "\t" << emp.hours << endl;
    }
    file.close();
}

void showReport(const string& filename)
{
    ifstream file(filename);
    if (!file)
    {
        cout << "Не удалось открыть отчет " << filename << endl;
        return;
    }

    cout << "Содержимое отчета:" << endl;
    string line;
    while (getline(file, line))
    {
        cout << line << endl;
    }
    file.close();
}

int main()
{
    system("chcp 1251 > NULL");

    cout << "Поиск Creater.exe" << endl;
    wstring creatorPath = findExeFile("Creater.exe");
    if (creatorPath.empty())
    {
        cout << "Creater.exe не найден" << endl;
        return 1;
    }

    cout << "Поиск Reporter.exe" << endl;
    wstring reporterPath = findExeFile("Reporter.exe");
    if (reporterPath.empty())
    {
        cout << "Reporter.exe не найден" << endl;
        return 1;
    }

    string binFileName;
    int recordCount;

    cout << "Введите имя бинарного файла: ";
    cin >> binFileName;
    cout << "Введите количество записей: ";
    cin >> recordCount;

    STARTUPINFOW siCreator;
    PROCESS_INFORMATION piCreator;

    ZeroMemory(&siCreator, sizeof(siCreator));
    siCreator.cb = sizeof(siCreator);
    ZeroMemory(&piCreator, sizeof(piCreator));

    wstring creatorCmdLine = L"\"" + creatorPath + L"\" " +
        stringToWstring(binFileName) + L" " +
        to_wstring(recordCount);

    wchar_t* creatorCmdLineMutable = new wchar_t[creatorCmdLine.length() + 1];
    wcscpy_s(creatorCmdLineMutable, creatorCmdLine.length() + 1, creatorCmdLine.c_str());

    string creatorCmdLineA(creatorCmdLine.begin(), creatorCmdLine.end());
    cout << "Запуск Creator (команда: " << creatorCmdLineA << ")" << endl;

    if (!CreateProcessW(
        NULL,                   
        creatorCmdLineMutable,   
        NULL,                    
        NULL,                    
        FALSE,                   
        0,        
        NULL,                    
        NULL,                    
        &siCreator,              
        &piCreator))             
    {
        cout << "Ошибка при запуске Creator. Код ошибки: " << GetLastError() << endl;
        delete[] creatorCmdLineMutable;
        return 1;
    }

    delete[] creatorCmdLineMutable;

    cout << "Процесс Creator запущен. Ожидание завершения..." << endl;

    WaitForSingleObject(piCreator.hProcess, INFINITE);

    cout << "Процесс Creator завершен." << endl;

    CloseHandle(piCreator.hProcess);
    CloseHandle(piCreator.hThread);

    ifstream testFile(binFileName);
    if (!testFile)
    {
        cout << "Файл " << binFileName << " не был создан" << endl;
        return 1;
    }
    testFile.close();

    showBinaryFile(binFileName);

    string reportFileName;
    double hourlyRate;

    cout << "Введите имя файла отчета: ";
    cin >> reportFileName;
    cout << "Введите оплату за час: ";
    cin >> hourlyRate;

    STARTUPINFOW siReporter;
    PROCESS_INFORMATION piReporter;

    ZeroMemory(&siReporter, sizeof(siReporter));
    siReporter.cb = sizeof(siReporter);
    ZeroMemory(&piReporter, sizeof(piReporter));

    wstring reporterCmdLine = L"\"" + reporterPath + L"\" " +
        stringToWstring(binFileName) + L" " +
        stringToWstring(reportFileName) + L" " +
        to_wstring(hourlyRate);

    wchar_t* reporterCmdLineMutable = new wchar_t[reporterCmdLine.length() + 1];
    wcscpy_s(reporterCmdLineMutable, reporterCmdLine.length() + 1, reporterCmdLine.c_str());

    string reporterCmdLineA(reporterCmdLine.begin(), reporterCmdLine.end());
    cout << "Запуск Reporter... (команда: " << reporterCmdLineA << ")" << endl;

    if (!CreateProcessW(
        NULL,                     
        reporterCmdLineMutable,   
        NULL,                     
        NULL,                     
        FALSE,                    
        0,         
        NULL,                     
        NULL,                    
        &siReporter,            
        &piReporter))             
    {
        cout << "Ошибка при запуске Reporter. Код ошибки: " << GetLastError() << endl;
        delete[] reporterCmdLineMutable;
        return 1;
    }

    delete[] reporterCmdLineMutable;

    cout << "Процесс Reporter запущен. Ожидание завершения" << endl;

    WaitForSingleObject(piReporter.hProcess, INFINITE);

    cout << "Процесс Reporter завершен." << endl;

    CloseHandle(piReporter.hProcess);
    CloseHandle(piReporter.hThread);

    showReport(reportFileName);
    return 0;
}
