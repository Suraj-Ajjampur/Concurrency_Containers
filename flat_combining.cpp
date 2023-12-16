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

#include "flat_combining.h"

void SGLQueue::enqueue(int val) {
    int index = 0; // Determine the index for this thread, e.g., thread ID modulo array size
    combiningArray[index].value.store(val, std::memory_order_relaxed);
    combiningArray[index].pending.store(true, std::memory_order_release);

    std::unique_lock<std::mutex> lock(sgl);
    if (lock.owns_lock()) {
        combine();
    }
}

int SGLQueue::dequeue() {
    int index = 0; // Determine the index for this thread, e.g., thread ID modulo array size
    combiningArray[index].pending.store(true, std::memory_order_release);

    std::unique_lock<std::mutex> lock(sgl);
    if (lock.owns_lock()) {
        combine();
    }

    while (!combiningArray[index].completed.load(std::memory_order_acquire)) {
        // Optionally, use a condition variable to wait
    }

    return combiningArray[index].retValue.load(std::memory_order_relaxed);
}

void SGLQueue::combine() {
    for (auto& op : combiningArray) {
        if (op.pending.load(std::memory_order_acquire)) {
            if (!op.completed.load(std::memory_order_relaxed)) {
                // Check if it's an enqueue or dequeue operation
                // For simplicity, let's assume a negative value indicates a dequeue request
                if (op.value.load(std::memory_order_relaxed) >= 0) {
                    q.push(op.value.load(std::memory_order_relaxed));
                } else {
                    if (!q.empty()) {
                        op.retValue.store(q.front(), std::memory_order_relaxed);
                        q.pop();
                    } else {
                        op.retValue.store(-1, std::memory_order_relaxed); // Indicate empty queue
                    }
                }
                op.pending.store(false, std::memory_order_relaxed);
                op.completed.store(true, std::memory_order_release);
            }
        }
    }
}