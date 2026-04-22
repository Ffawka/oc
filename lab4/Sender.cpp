#include "common.h"
#include <iostream>
#include <string>

class Sender {
private:
    std::string filename;
    int sender_id;
    HANDLE ready_event;

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

    void writeMessage(int index, const Message& msg) {
        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(sizeof(QueueHeader) + index * sizeof(Message));
        file.write(reinterpret_cast<const char*>(&msg), sizeof(Message));
        file.close();
    }

    bool sendMessage(const std::string& text) {
        QueueHeader header = readHeader();

        if (header.count >= header.max_size) {
            return false; 
        }

        Message msg;
        strncpy(msg.text, text.c_str(), MAX_MSG_LEN);
        msg.text[MAX_MSG_LEN] = '\0';
        msg.sender_id = sender_id;
        msg.is_valid = true;

        writeMessage(header.tail, msg);

        header.tail = (header.tail + 1) % header.max_size;
        header.count++;
        writeHeader(header);

        return true;
    }

public:
    Sender(int argc, char* argv[]) {
        if (argc < 3) {
            std::cerr << "Использование: Sender.exe <filename> <sender_id>" << std::endl;
            exit(1);
        }

        filename = argv[1];
        sender_id = atoi(argv[2]);

        std::string event_name = "ReadyEvent_" + std::to_string(sender_id);
        ready_event = OpenEventA(EVENT_MODIFY_STATE, FALSE, event_name.c_str());

        if (ready_event == NULL) {
            std::cerr << "Ошибка открытия события готовности!" << std::endl;
            exit(1);
        }
    }

    ~Sender() {
        CloseHandle(ready_event);
    }

    void run() {
        SetEvent(ready_event);
        std::cout << "Sender " << sender_id << " готов к работе!" << std::endl;

        std::string command;
        while (true) {
            std::cout << "\n[Sender " << sender_id << "] Введите команду (send - отправить сообщение, exit - завершить работу): ";
            std::cin >> command;

            if (command == "send") {
                std::cout << "Введите сообщение (макс. " << MAX_MSG_LEN << " символов): ";
                std::cin.ignore();
                std::string message;
                std::getline(std::cin, message);

                if (message.length() > MAX_MSG_LEN) {
                    std::cout << "Сообщение слишком длинное! Усечение до " << MAX_MSG_LEN << " символов." << std::endl;
                    message = message.substr(0, MAX_MSG_LEN);
                }

                if (sendMessage(message)) {
                    std::cout << "Сообщение отправлено в очередь!" << std::endl;
                }
                else {
                    std::cout << "Очередь заполнена. Ожидание освобождения места..." << std::endl;
                    Sleep(100);
                }
            }
            else if (command == "exit") {
                std::cout << "Завершение работы Sender " << sender_id << "..." << std::endl;
                break;
            }
            else {
                std::cout << "Неизвестная команда!" << std::endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    Sender sender(argc, argv);
    sender.run();

    return 0;
}
