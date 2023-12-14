#ifndef MSQ_H
#define MSQ_H

#include "my_atomics.h"
#include <assert.h>
#define DUMMY 0

class msqueue {
public:
    class node {
    public:
        node(int v) : val(v), next(nullptr) {}
        int val;
        std::atomic<node*> next;
    };

    std::atomic<node*> head, tail;
    msqueue();
    void enqueue(int val);
    int dequeue();
};

void testBasicQueueOperations();
void testConcurrentuQeueOperations();

#endif //MSQ_H