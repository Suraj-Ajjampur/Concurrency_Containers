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


#include "my_atomics.h"
#include <mutex>
#include <list>
#include <vector>
#include <queue>
#include <assert.h>




class SGLQueue {
private:
    std::mutex sgl;
    std::queue<int> q;

    struct CombiningOp {
        std::atomic<bool> pending;
        std::atomic<bool> completed;
        std::atomic<int> value;
        std::atomic<int> retValue; // For dequeue operations

        CombiningOp() : pending(false), completed(false), value(0), retValue(0) {}
    };

    std::vector<CombiningOp> combiningArray; // Size should be based on expected concurrency level

public:
    SGLQueue(int maxConcurrency) : combiningArray(maxConcurrency) {}

    void enqueue(int val);
    int dequeue();
    void combine();
};



#endif // FLAT_COMBINING_H
