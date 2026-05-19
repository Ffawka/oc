#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include "employee.h"

using namespace std;

class EmployeeServer 
{
private:
    string filename;
    fstream file;
    HANDLE hPipe;
    int locked_record;      

public:
    EmployeeServer() : hPipe(INVALID_HANDLE_VALUE), locked_record(-1) {}

    void lockRecord(int id) 
    {
        locked_record = id;
        cout << "[Сервер] Запись с ID = " << id << " заблокирована" << endl;
    }

    void unlockRecord() {
        if (locked_record != -1) 
        {
            cout << "[Сервер] Запись с ID=" << locked_record << " разблокирована" << endl;
        }
        locked_record = -1;
    }

    bool isLocked(int id) 
    {
        return (locked_record != -1 && locked_record == id);
    }

    bool findRecord(int id, Employee& emp) 
    {
        file.clear();
        file.seekg(0, ios::beg);

        Employee e;
        while (file.read(reinterpret_cast<char*>(&e), sizeof(Employee))) 
        {
            if (e.num == id) 
            {
                emp = e;
                return true;
            }
        }
        return false;
    }

    void updateRecord(int id, const Employee& newEmp) 
    {
        file.clear();
        file.seekg(0, ios::beg);

        Employee e;
        streampos pos = 0;

        while (file.read(reinterpret_cast<char*>(&e), sizeof(Employee))) 
        {
            if (e.num == id) 
            {
                file.seekp(pos, ios::beg);
                file.write(reinterpret_cast<const char*>(&newEmp), sizeof(Employee));
                file.flush();
                cout << "[Сервер] Запись с ID=" << id << " обновлена" << endl;
                break;
            }
            pos = file.tellg();
        }
    }

    void displayFile() 
    {
        file.clear();
        file.seekg(0, ios::beg);

        Employee e;
        cout << "\nСодержимое файла = " << endl;
        while (file.read(reinterpret_cast<char*>(&e), sizeof(Employee))) {
            cout << "ID: " << e.num << " Имя: " << e.name << " Часы: " << e.hours << endl;
        }
    }

    void createFile() 
    {
        cout << "Введите имя бинарного файла: ";
        cin >> filename;

        int count;
        cout << "Введите количество сотрудников: ";
        cin >> count;

        file.open(filename, ios::binary | ios::out);
        if (!file) 
        {
            cerr << "Ошибка создания файла!" << endl;
            exit(1);
        }

        for (int i = 0; i < count; i++) 
        {
            Employee emp;
            cout << "\nСотрудник " << (i + 1) << ":" << endl;
            cout << "  ID: ";
            cin >> emp.num;
            cout << "  Имя: ";
            cin >> emp.name;
            cout << "  Отработанные часы: ";
            cin >> emp.hours;
            file.write(reinterpret_cast<const char*>(&emp), sizeof(Employee));
        }

        file.close();
        file.open(filename, ios::binary | ios::in | ios::out);

        cout << "\nФайл создан" << endl;
        displayFile();
    }

    bool processRequest(const Request& req, Response& res) 
    {
        memset(&res, 0, sizeof(Response));

        if (req.type == OP_EXIT) 
        {
            cout << "[Сервер] Получен сигнал завершения от клиента" << endl;
            res.status = 0;
            return false; 
        }

        if (req.type == OP_READ) 
        {
            cout << "[Сервер] Запрос на чтение ID=" << req.key << endl;

            if (isLocked(req.key)) 
            {
                cout << "[Сервер] запись заблокирована, чтение невозможно" << endl;
                res.status = -1;
                return true;
            }

            if (findRecord(req.key, res.emp)) 
            {
                res.status = 0;
                cout << "[Сервер] Запись найдена, отправляю клиенту" << endl;
            }
            else 
            {
                res.status = -1;
                cout << "[Сервер] Запись с ID=" << req.key << " не найдена" << endl;
            }
            return true;
        }

        if (req.type == OP_UPDATE) 
        {
            cout << "[Сервер] Запрос на обновление ID=" << req.key << endl;


            if (isLocked(req.key) && locked_record != req.key) {
                cout << "[Сервер] запись заблокирована другим процессом" << endl;
                res.status = -1;
                return true;
            }

            if (locked_record == -1) 
            {
                lockRecord(req.key);
            }

            if (findRecord(req.key, res.emp)) 
            {
                res.status = 0;
                cout << "[Сервер] Текущая запись отправлена клиенту" << endl;
            }
            else 
            {
                res.status = -1;
                cout << "[Сервер] Запись с ID=" << req.key << " не найдена" << endl;
            }
            return true;
        }

        return true;
    }

    void run() {
        cout << "Сервер запущен" << endl;

        createFile();

        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

        hPipe = CreateNamedPipeW(
            PIPE_NAME,                   
            PIPE_ACCESS_DUPLEX,           
            PIPE_TYPE_MESSAGE |           
            PIPE_READMODE_MESSAGE |
            PIPE_WAIT,
            1,                           
            sizeof(Request),              
            sizeof(Response),            
            0,                            
            &sa
        );

        if (hPipe == INVALID_HANDLE_VALUE) 
        {
            cerr << "Ошибка создания канала: " << GetLastError() << endl;
            file.close();
            return;
        }

        cout << "Именованный канал создан. Подключения клиента" << endl;

        BOOL connected = ConnectNamedPipe(hPipe, NULL);
        if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) 
        {
            cerr << "Ошибка подключения клиента: " << GetLastError() << endl;
            CloseHandle(hPipe);
            file.close();
            return;
        }

        cout << "Клиент подключен" << endl << endl;

        Request req;
        Response res;
        DWORD bytesRead, bytesWritten;

        while (true) 
        {
            BOOL success = ReadFile(hPipe, &req, sizeof(Request), &bytesRead, NULL);
            if (!success || bytesRead == 0) 
            {
                cout << "Клиент отключился" << endl;
                break;
            }

            bool continueServer = processRequest(req, res);

            WriteFile(hPipe, &res, sizeof(Response), &bytesWritten, NULL);

            if (req.type == OP_UPDATE && res.status == 0) 
            {
                char confirm[10];
                cout << "[Сервер] Ожидание команды от клиента..." << endl;
                ReadFile(hPipe, confirm, sizeof(confirm), &bytesRead, NULL);

                if (strcmp(confirm, "update") == 0) 
                {
                    Employee newEmp;
                    ReadFile(hPipe, &newEmp, sizeof(Employee), &bytesRead, NULL);
                    updateRecord(req.key, newEmp);
                    cout << "[Сервер] Запись модифицирована" << endl;
                }
                else if (strcmp(confirm, "cancel") == 0) 
                {
                    cout << "[Сервер] Модификация отменена клиентом" << endl;
                }

                unlockRecord();
            }

            if (!continueServer) 
            {
                break;
            }
        }

        cout << "\nФайл после работы клиента = " << endl;
        displayFile();

        CloseHandle(hPipe);
        file.close();

        cout << "Сервер завершил работу" << endl;
    }
};

int main() 
{
    setlocale(LC_ALL, "Russian");

    EmployeeServer server;
    server.run();
    return 0;
}
