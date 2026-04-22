#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

CRITICAL_SECTION g_csArray;
CRITICAL_SECTION g_csConsole;

int* g_array = nullptr;
int g_arraySize = 0;
int g_markerCount = 0;

HANDLE* g_hStartEvents = nullptr;
HANDLE* g_hContinueEvents = nullptr;
HANDLE* g_hTerminateEvents = nullptr;
HANDLE* g_hStoppedEvents = nullptr;
HANDLE* g_hThreadHandles = nullptr;

struct MarkerParams
{
    int id;
    int markedCount;
    int lastFailedIndex;
    bool isActive;
    bool hasStopped;
};

MarkerParams* g_markerParams = nullptr;

void TestArrayInitialization() 
{
    cout << "\nТест : Инициализация массива" << endl;

    int testSize = 10;
    int* testArray = new int[testSize];

    for (int i = 0; i < testSize; i++) 
    {
        testArray[i] = 0;
    }

    bool allZero = true;
    for (int i = 0; i < testSize; i++) 
    {
        if (testArray[i] != 0) 
        {
            allZero = false;
            break;
        }
    }

    if (allZero) 
    {
        cout << "Массив успешно инициализирован нулями" << endl;
    }
    else {
        cout << "Ошибка инициализации массива" << endl;
    }

    delete[] testArray;
}

void TestMarkerStructure() 
{
    cout << "\nТест : Структура параметров маркера" << endl;

    MarkerParams testMarker;
    testMarker.id = 0;
    testMarker.markedCount = 0;
    testMarker.lastFailedIndex = -1;
    testMarker.isActive = true;
    testMarker.hasStopped = false;

    bool passed = true;

    if (testMarker.markedCount != 0) 
    {
        cout << "Неверное начальное количество помеченных элементов" << endl;
        passed = false;
    }

    if (testMarker.lastFailedIndex != -1) 
    {
        cout << "Неверный начальный индекс ошибки" << endl;
        passed = false;
    }

    if (!testMarker.isActive) 
    {
        cout << "Маркер должен быть активным" << endl;
        passed = false;
    }

    if (testMarker.hasStopped) 
    {
        cout << "Маркер не должен быть остановлен" << endl;
        passed = false;
    }

    if (passed) 
    {
        cout << "Структура маркера работает корректно" << endl;
    }
}

void RunTests() 
{
    cout << "\nЗапуск тестов" << endl;

    TestArrayInitialization();
    TestMarkerStructure();

    cout << "\nТесты завершены" << endl;
}

DWORD WINAPI MarkerThread(LPVOID lpParam)
{
    int id = *(int*)lpParam;
    delete (int*)lpParam;

    srand(id);

    WaitForSingleObject(g_hStartEvents[id], INFINITE);

    bool running = true;

    while (running)
    {
        int randomIndex = rand() % g_arraySize;

        EnterCriticalSection(&g_csArray);
        if (g_array[randomIndex] == 0)
        {
            g_array[randomIndex] = id;
            g_markerParams[id].markedCount++;
            LeaveCriticalSection(&g_csArray);

            Sleep(5);
        }
        else
        {
            LeaveCriticalSection(&g_csArray);

            g_markerParams[id].lastFailedIndex = randomIndex;

            EnterCriticalSection(&g_csConsole);
            cout << "Поток " << id
                << ": помечено элементов = " << g_markerParams[id].markedCount
                << ", невозможно пометить индекс " << randomIndex << endl;
            LeaveCriticalSection(&g_csConsole);

            g_markerParams[id].hasStopped = true;
            SetEvent(g_hStoppedEvents[id]);

            HANDLE events[2] = { g_hContinueEvents[id], g_hTerminateEvents[id] };
            DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0 + 1)
            {
                running = false;
            }

            g_markerParams[id].hasStopped = false;
        }
    }

    EnterCriticalSection(&g_csArray);
    for (int i = 0; i < g_arraySize; i++)
    {
        if (g_array[i] == id)
        {
            g_array[i] = 0;
        }
    }
    LeaveCriticalSection(&g_csArray);

    EnterCriticalSection(&g_csConsole);
    cout << "Поток " << id << " завершил работу, очистил "
        << g_markerParams[id].markedCount << " элементов" << endl;
    LeaveCriticalSection(&g_csConsole);

    return 0;
}

int main() {
    setlocale(LC_ALL, "Russian");

    RunTests();

    cout << "\nНажмите Enter для продолжения основной программы...";
    cin.get();
    cin.get(); 

    InitializeCriticalSection(&g_csArray);
    InitializeCriticalSection(&g_csConsole);

    cout << "\nВведите размер массива: ";
    cin >> g_arraySize;

    if (g_arraySize <= 0)
    {
        cout << "Размер массива должен быть положительным!" << endl;
        return 1;
    }

    g_array = new int[g_arraySize];

    for (int i = 0; i < g_arraySize; i++)
    {
        g_array[i] = 0;
    }

    cout << "Введите количество потоков marker: ";
    cin >> g_markerCount;

    if (g_markerCount <= 0)
    {
        cout << "Количество потоков должно быть положительным!" << endl;
        delete[] g_array;
        return 1;
    }

    g_hStartEvents = new HANDLE[g_markerCount];
    g_hContinueEvents = new HANDLE[g_markerCount];
    g_hTerminateEvents = new HANDLE[g_markerCount];
    g_hStoppedEvents = new HANDLE[g_markerCount];
    g_hThreadHandles = new HANDLE[g_markerCount];
    g_markerParams = new MarkerParams[g_markerCount];

    for (int i = 0; i < g_markerCount; i++)
    {
        g_hStartEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        g_hContinueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        g_hTerminateEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        g_hStoppedEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        g_markerParams[i].id = i;
        g_markerParams[i].markedCount = 0;
        g_markerParams[i].lastFailedIndex = -1;
        g_markerParams[i].isActive = true;
        g_markerParams[i].hasStopped = false;
    }

    for (int i = 0; i < g_markerCount; i++)
    {
        int* param = new int(i);
        g_hThreadHandles[i] = CreateThread(NULL, 0, MarkerThread, param, 0, NULL);
        if (g_hThreadHandles[i] == NULL)
        {
            cout << "Ошибка создания потока " << i << endl;
            return 1;
        }
    }

    cout << "\nЗапуск всех потоков marker..." << endl;
    for (int i = 0; i < g_markerCount; i++)
    {
        SetEvent(g_hStartEvents[i]);
    }

    int activeMarkers = g_markerCount;

    while (activeMarkers > 0)
    {
        vector<HANDLE> stoppedEvents;
        vector<int> stoppedIds;

        for (int i = 0; i < g_markerCount; i++)
        {
            if (g_markerParams[i].isActive)
            {
                stoppedEvents.push_back(g_hStoppedEvents[i]);
                stoppedIds.push_back(i);
            }
        }

        if (!stoppedEvents.empty())
        {
            WaitForMultipleObjects(stoppedEvents.size(), stoppedEvents.data(), TRUE, INFINITE);
        }
        else
        {
            break;
        }

        EnterCriticalSection(&g_csConsole);
        cout << "\nСодержимое массива = " << endl;
        for (int i = 0; i < g_arraySize; i++)
        {
            cout << g_array[i] << " ";
            if ((i + 1) % 20 == 0 && i != g_arraySize - 1)
            {
                cout << endl;
            }
        }
        cout << endl;
        LeaveCriticalSection(&g_csConsole);

        int terminateId;
        cout << "Введите номер потока для завершения (0-" << g_markerCount - 1 << "): ";
        cin >> terminateId;

        if (terminateId < 0 || terminateId >= g_markerCount || !g_markerParams[terminateId].isActive)
        {
            EnterCriticalSection(&g_csConsole);
            cout << "Неверный номер потока или поток уже завершен!" << endl;
            LeaveCriticalSection(&g_csConsole);

            for (int i = 0; i < g_markerCount; i++)
            {
                if (g_markerParams[i].isActive && g_markerParams[i].hasStopped)
                {
                    ResetEvent(g_hStoppedEvents[i]);
                    SetEvent(g_hContinueEvents[i]);
                }
            }
            continue;
        }

        SetEvent(g_hTerminateEvents[terminateId]);

        WaitForSingleObject(g_hThreadHandles[terminateId], INFINITE);
        CloseHandle(g_hThreadHandles[terminateId]);
        g_markerParams[terminateId].isActive = false;
        activeMarkers--;

        EnterCriticalSection(&g_csConsole);
        cout << "\nСодержимое массива после завершения потока " << terminateId << endl;
        for (int i = 0; i < g_arraySize; i++)
        {
            cout << g_array[i] << " ";
            if ((i + 1) % 20 == 0 && i != g_arraySize - 1)
            {
                cout << endl;
            }
        }
        cout << endl;
        LeaveCriticalSection(&g_csConsole);

        for (int i = 0; i < g_markerCount; i++)
        {
            if (g_markerParams[i].isActive && g_markerParams[i].hasStopped)
            {
                ResetEvent(g_hStoppedEvents[i]);
                SetEvent(g_hContinueEvents[i]);
            }
        }
    }

    cout << "\nВсе потоки завершили работу." << endl;

    for (int i = 0; i < g_markerCount; i++)
    {
        if (g_hStartEvents[i]) CloseHandle(g_hStartEvents[i]);
        if (g_hContinueEvents[i]) CloseHandle(g_hContinueEvents[i]);
        if (g_hTerminateEvents[i]) CloseHandle(g_hTerminateEvents[i]);
        if (g_hStoppedEvents[i]) CloseHandle(g_hStoppedEvents[i]);
    }

    delete[] g_array;
    delete[] g_hStartEvents;
    delete[] g_hContinueEvents;
    delete[] g_hTerminateEvents;
    delete[] g_hStoppedEvents;
    delete[] g_hThreadHandles;
    delete[] g_markerParams;

    DeleteCriticalSection(&g_csArray);
    DeleteCriticalSection(&g_csConsole);

    system("pause");
    return 0;
}
