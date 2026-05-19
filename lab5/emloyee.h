#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <windows.h>
#include <string>

#define PIPE_NAME LR"(\\.\pipe\employee_pipe)"
#define NAME_SIZE 10

#pragma pack(push, 1)
struct Employee {
    int num;                 
    char name[NAME_SIZE];    
    double hours;            
};
#pragma pack(pop)

enum OperationType 
{
    OP_READ = 1,
    OP_UPDATE = 2,
    OP_EXIT = 3
};

struct Request 
{
    int type;               
    int key;                
    Employee emp;           
};

struct Response 
{
    int status;             
    Employee emp;
};

#endif
