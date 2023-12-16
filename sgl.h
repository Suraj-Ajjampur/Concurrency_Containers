/********************************************************************
 * @author Suraj Ajjampur
 * @file sgl.h
 * 
 * @brief This header file implements a Stack and Queue classes to be used 
 * in a multithreaded application using a Single-Global Lock
 * 
 * @date 14 Dec 2023
********************************************************************/

#ifndef SGL_H
#define SGL_H

#include "my_atomics.h"
#include <mutex>
#include <list>
#include <assert.h>


class SGLQueue{
    private:
        std::mutex sgl;
        std::list<int> q;
    public:
        void enqueue(int val);
        int dequeue();
};

class SGLStack{
    private:
        std::mutex sgl;
        std::list<int> q;
    public:
        void push(int val);
        int pop();
};



void testBasicSGLQueueOperations();
void testConcurrentSGLQueueOperations();

void testBasicSGLStackOperations();
void testConcurrentSGLStackOperations();

#endif