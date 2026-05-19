#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>
#include "employee.h"

using namespace std;

class EmployeeClient
{
private:
    HANDLE hPipe;

public:
    EmployeeClient() : hPipe(INVALID_HANDLE_VALUE) {}

    bool connect()
    {
        hPipe = CreateFileW(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            cerr << "Ошибка подключения к каналу. Убедитесь, что сервер запущен." << endl;
            return false;
        }

        DWORD mode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);

        cout << "Подключение к серверу установлено" << endl << endl;
        return true;
    }

    void readRecord()
    {
        Request req;
        Response res;
        DWORD bytesRead, bytesWritten;

        cout << "\nЧтение записи = " << endl;
        cout << "Введите ID сотрудника: ";
        cin >> req.key;
        req.type = OP_READ;

        WriteFile(hPipe, &req, sizeof(Request), &bytesWritten, NULL);

        ReadFile(hPipe, &res, sizeof(Response), &bytesRead, NULL);

        if (res.status == 0)
        {
            cout << "\nДанные сотрудника = " << endl;
            cout << "ID: " << res.emp.num << endl;
            cout << "Имя: " << res.emp.name << endl;
            cout << "Часы: " << res.emp.hours << endl;
        }
        else {
            cout << "Ошибка: запись с ID = " << req.key << " не найдена или заблокирована" << endl;
        }
    }

    void updateRecord()
    {
        Request req;
        Response res;
        DWORD bytesRead, bytesWritten;

        cout << "\nОбновление записи" << endl;
        cout << "Введите ID сотрудника для обновления: ";
        cin >> req.key;
        req.type = OP_UPDATE;

        WriteFile(hPipe, &req, sizeof(Request), &bytesWritten, NULL);

        ReadFile(hPipe, &res, sizeof(Response), &bytesRead, NULL);

        if (res.status != 0) {
            cout << "Ошибка: запись с ID = " << req.key << " не найдена или заблокирована" << endl;
            return;
        }

        cout << "\nТекущие данные" << endl;
        cout << "ID: " << res.emp.num << endl;
        cout << "Имя: " << res.emp.name << endl;
        cout << "Часы: " << res.emp.hours << endl;

        Employee newEmp;
        newEmp.num = res.emp.num;

        cout << "\nВведите новые данные:" << endl;
        cout << "Новое имя: ";
        cin >> newEmp.name;
        cout << "Новое количество часов: ";
        cin >> newEmp.hours;

        cout << "\nПодтвердить изменение? (y/n): ";
        char confirm;
        cin >> confirm;

        if (confirm == 'y' || confirm == 'Y')
        {
            WriteFile(hPipe, "update", 7, &bytesWritten, NULL);
            WriteFile(hPipe, &newEmp, sizeof(Employee), &bytesWritten, NULL);
            cout << "Данные отправлены на сервер для обновления" << endl;
        }
        else
        {
            WriteFile(hPipe, "cancel", 7, &bytesWritten, NULL);
            cout << "Операция обновления отменена" << endl;
        }
    }

    void run()
    {
        if (!connect())
        {
            system("pause");
            return;
        }

        int choice;

        while (true) {
            cout << "\nМеню" << endl;
            cout << "1. Чтение записи" << endl;
            cout << "2. Модификация записи" << endl;
            cout << "3. Выход" << endl;
            cout << "Выберите операцию: ";
            cin >> choice;
            cin.ignore();

            if (choice == 3)
            {
                Request req;
                req.type = OP_EXIT;
                req.key = 0;
                DWORD bytesWritten;
                WriteFile(hPipe, &req, sizeof(Request), &bytesWritten, NULL);
                cout << "Завершение работы клиента" << endl;
                break;
            }

            switch (choice)
            {
            case 1:
                readRecord();
                break;
            case 2:
                updateRecord();
                break;
            default:
                cout << "Неверный выбор. Попробуйте снова" << endl;
                break;
            }

            cout << "\nНажмите Enter для продолжения";
            cin.get();
        }

        CloseHandle(hPipe);
        cout << "Клиент завершил работу" << endl;
    }
};

int main()
{
    setlocale(LC_ALL, "Russian");

    EmployeeClient client;
    client.run();

    system("pause");
    return 0;
}
