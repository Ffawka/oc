#include "common.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <synchapi.h>

class Receiver {
private:
    std::string filename;
    int queue_size;
    int num_senders;
    std::vector<PROCESS_INFORMATION> sender_processes;
    HANDLE* sender_ready_events;

    bool createQueueFile() {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file) {
            std::cerr << "Ошибка создания файла!" << std::endl;
            return false;
        }

        QueueHeader header;
        header.head = 0;
        header.tail = 0;
        header.count = 0;
        header.max_size = queue_size;

        file.write(reinterpret_cast<char*>(&header), sizeof(QueueHeader));

        Message empty_msg;
        empty_msg.is_valid = false;
        empty_msg.text[0] = '\0';
        empty_msg.sender_id = -1;

        for (int i = 0; i < queue_size; i++) {
            file.write(reinterpret_cast<char*>(&empty_msg), sizeof(Message));
        }

        file.close();
        return true;
    }

    QueueHeader readHeader() {
        std::ifstream file(filename, std::ios::binary);
        QueueHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(QueueHeader));
        file.close();
        return header;
    }

    void writeHeader(const QueueHeader& header) {
        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(0, std::ios::beg);
        file.write(reinterpret_cast<const char*>(&header), sizeof(QueueHeader));
        file.close();
    }

    Message readMessage(int index) {
        std::ifstream file(filename, std::ios::binary);
        file.seekg(sizeof(QueueHeader) + index * sizeof(Message));
        Message msg;
        file.read(reinterpret_cast<char*>(&msg), sizeof(Message));
        file.close();
        return msg;
    }

    bool readNextMessage(Message& msg) {
        QueueHeader header = readHeader();

        if (header.count == 0) {
            return false; 
        }

        msg = readMessage(header.head);

        header.head = (header.head + 1) % header.max_size;
        header.count--;
        writeHeader(header);

        return true;
    }

    bool startSenders() {
        sender_ready_events = new HANDLE[num_senders];

        for (int i = 0; i < num_senders; i++) {
            std::string event_name = "ReadyEvent_" + std::to_string(i);
            sender_ready_events[i] = CreateEventA(NULL, FALSE, FALSE, event_name.c_str());

            std::string cmd_line = "Sender.exe " + filename + " " + std::to_string(i);

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            char* cmd_line_cstr = new char[cmd_line.length() + 1];
            strcpy(cmd_line_cstr, cmd_line.c_str());

            if (!CreateProcessA(NULL, cmd_line_cstr, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                std::cerr << "Ошибка создания процесса Sender " << i << std::endl;
                delete[] cmd_line_cstr;
                return false;
            }

            delete[] cmd_line_cstr;
            sender_processes.push_back(pi);
        }

        std::cout << "Ожидание готовности всех отправителей..." << std::endl;
        WaitForMultipleObjects(num_senders, sender_ready_events, TRUE, INFINITE);
        std::cout << "Все отправители готовы к работе!" << std::endl;

        return true;
    }

public:
    void run() {
        std::cout << "Введите имя бинарного файла: ";
        std::cin >> filename;

        std::cout << "Введите количество записей в бинарном файле: ";
        std::cin >> queue_size;

        if (queue_size > MAX_QUEUE_SIZE) {
            std::cout << "Количество записей не может превышать " << MAX_QUEUE_SIZE << std::endl;
            queue_size = MAX_QUEUE_SIZE;
        }

        if (!createQueueFile()) {
            return;
        }

        std::cout << "Введите количество процессов Sender: ";
        std::cin >> num_senders;

        if (!startSenders()) {
            return;
        }

        std::string command;
        while (true) {
            std::cout << "\nВведите команду (read - прочитать сообщение, exit - завершить работу): ";
            std::cin >> command;

            if (command == "read") {
                Message msg;
                if (readNextMessage(msg)) {
                    std::cout << "Получено сообщение от Sender " << msg.sender_id
                        << ": " << msg.text << std::endl;
                }
                else {
                    std::cout << "Очередь пуста. Ожидание сообщений..." << std::endl;
                    Sleep(100);
                }
            }
            else if (command == "exit") {
                std::cout << "Завершение работы Receiver..." << std::endl;
                break;
            }
            else {
                std::cout << "Неизвестная команда!" << std::endl;
            }
        }

        for (auto& pi : sender_processes) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        for (int i = 0; i < num_senders; i++) {
            CloseHandle(sender_ready_events[i]);
        }
        delete[] sender_ready_events;
    }
};

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    Receiver receiver;
    receiver.run();

    return 0;
}
