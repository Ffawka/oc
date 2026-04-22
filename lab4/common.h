#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <windows.h>
#include <synchapi.h>

const int MAX_MSG_LEN = 20;
const int MAX_QUEUE_SIZE = 100; 

struct Message 
{
    char text[MAX_MSG_LEN + 1]; 
    int sender_id;               
    bool is_valid;               
};

struct QueueHeader 
{
    int head;        
    int tail;        
    int count;      
    int max_size;   
};

#endif
