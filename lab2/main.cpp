#include <iostream>
#include <windows.h>
#include <climits>
#include <string>
#include <cstdlib>  

using namespace std;

int* arr;
int g_size;  
int minElement, maxElement;
double averageValue;
bool minMaxDone = false;
bool averageDone = false;

CRITICAL_SECTION cs;

DWORD WINAPI min_max(LPVOID lpParam)
{
    cout << "Поток min_max запущен" << endl;

    minElement = INT_MAX;
    maxElement = INT_MIN;

    for (int i = 0; i < g_size; i++)
    {
        if (arr[i] < minElement)
        {
            minElement = arr[i];
        }
        if (arr[i] > maxElement)
        {
            maxElement = arr[i];
        }
        cout << "min_max arr[" << i << "] = " << arr[i] << ", min = " << minElement << ", max = " << maxElement << endl;

        Sleep(7);
    }
    cout << "min = " << minElement << endl;
    cout << "max = " << maxElement << endl;

    EnterCriticalSection(&cs);
    minMaxDone = true;
    LeaveCriticalSection(&cs);

    return 0;
}

DWORD WINAPI average(LPVOID lpParam)
{
    cout << "Поток average запущен" << endl;

    int sum = 0;
    for (int i = 0; i < g_size; i++)
    {
        sum += arr[i];
        cout << "average arr[" << i << "] = " << arr[i] << ", sum = " << sum << endl;

        Sleep(12);
    }
    averageValue = static_cast<double>(sum) / g_size;
    cout << "averageValue = " << averageValue << endl;

    EnterCriticalSection(&cs);
    averageDone = true;
    LeaveCriticalSection(&cs);

    return 0;
}

int main()
{
    setlocale(LC_ALL, "Russian");
    cout << "array size = " << endl;
    cin >> g_size;  

    if (g_size <= 0)
    {
        cout << "Error, size > 0" << endl;
        return 1;
    }

    cout << "size = " << g_size << endl;

    arr = new int[g_size];
    cout << "Enter " << g_size << " elements:" << endl;
    for (int i = 0; i < g_size; i++)
    {
        cin >> arr[i];
    }

    cout << "Initial array:" << endl;
    for (int i = 0; i < g_size; i++)
    {
        cout << "arr[" << i << "] = " << arr[i] << endl;
    }

    InitializeCriticalSection(&cs);

    HANDLE hMinMax, hAverage;
    DWORD dwMinMaxId, dwAverageId;

    hMinMax = CreateThread
    (
        NULL,
        0,
        min_max,
        NULL,
        0,
        &dwMinMaxId
    );

    if (hMinMax == NULL)
    {
        cout << "Error Create min_max" << endl;
        delete[] arr;
        return 1;
    }

    hAverage = CreateThread
    (
        NULL,
        0,
        average,
        NULL,
        0,
        &dwAverageId
    );

    if (hAverage == NULL)
    {
        cout << "Error Create average" << endl;
        CloseHandle(hMinMax);
        delete[] arr;
        return 1;
    }

    cout << "Потоки созданы" << endl;

    WaitForSingleObject(hMinMax, INFINITE);
    WaitForSingleObject(hAverage, INFINITE);

    bool minReplaced = false;
    bool maxReplaced = false;

    for (int i = 0; i < g_size; i++)
    {
        if (!minReplaced && arr[i] == minElement)
        {
            arr[i] = static_cast<int>(averageValue);
            minReplaced = true;
            cout << "new min i = " << i << endl;

        }
        if (!maxReplaced && arr[i] == maxElement)
        {
            arr[i] = static_cast<int>(averageValue);
            maxReplaced = true;
            cout << "new max i = " << i << endl;
        }
        if (minReplaced && maxReplaced) break;
    }

    cout << "min = " << minElement << endl;
    cout << "max = " << maxElement << endl;
    cout << "average = " << averageValue << endl;

    cout << "array после замены min & max = " << endl;
    for (int i = 0; i < g_size; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;

    CloseHandle(hMinMax);
    CloseHandle(hAverage);

    DeleteCriticalSection(&cs);

    delete[] arr;

    return 0;
}
