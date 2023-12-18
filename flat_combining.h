/*****************************************************************
 * @author Suraj Ajjampur
 * @file   flat_combining.cpp
 * 
 * @brief This C++ source file implements concurrent containers which uses
 * a single global lock using the Flat combining Method.
 * 
 * 
 * @date 16 Dec 2023
********************************************************************/

#ifndef FLAT_COMBINING_H
#define FLAT_COMBINING_H

#include <atomic>
#include <numeric>
#include "my_atomics.h"
#include <mutex>
#include <list>
#include <vector>
#include <queue>
#include <assert.h>
#include <condition_variable>
#include <stack>



enum OperationType {
    PUSH,
    POP,
    ENQUEUE,
    DEQUEUE
};

struct CombiningOp {
    std::atomic<bool> pending;
    std::atomic<bool> completed;
    std::atomic<int> value;
    std::atomic<int> retValue; // For dequeue operations
    std::atomic<OperationType> operation; // Indicates whether to enqueue or dequeue

    CombiningOp() : pending(false), completed(false), value(0), retValue(0) {}
};


class SGLQueue_FC {
    private:
        std::mutex sgl;
        std::queue<int> q;
        std::vector<CombiningOp> combiningArray; // Size should be based on expected concurrency level
        std::condition_variable cv;

    public:
        SGLQueue_FC(int maxConcurrency) : combiningArray(maxConcurrency) {}

        void enqueue(int val);
        int dequeue();
        void combine();
};

class SGLStack_FC {
private:
    std::mutex sgl;
    std::stack<int> stk;

    struct CombiningOp {
        std::atomic<bool> pending;
        std::atomic<bool> completed;
        std::atomic<int> value;
        std::atomic<OperationType> operation;

        CombiningOp() : pending(false), completed(false), value(0), operation(PUSH) {}
    };

    std::vector<CombiningOp> combiningArray;
    std::condition_variable cv;

    void combine();

public:
    SGLStack_FC(int maxConcurrency) : combiningArray(maxConcurrency) {}

    void push(int val);
    int pop();  // Returns -1 if the stack is empty
};

void sgl_queue_fc_test(std::vector<int>& values, int numThreads);
void sgl_stack_fc_test(std::vector<int>& values, int numThreads);



#endif // FLAT_COMBINING_H
