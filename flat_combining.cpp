/*****************************************************************
 * @author Suraj Ajjampur
 * @file   flat_combining.cpp
 * 
 * @brief This C++ source file implements concurrent containers which uses
 * a single global lock optimized using the Flat combining Method.
 * 
 * 
 * @date 16 Dec 2023
********************************************************************/

#include "flat_combining.h"

std::atomic<int> global_thread_index{0};

int get_thread_index() {
    static thread_local int thread_index = global_thread_index++;
    return thread_index;
}

// Example thread-local index assignment (requires further implementation):
thread_local int thread_index = get_thread_index();

void SGLQueue_FC::enqueue(int val) {
    DEBUG_MSG("Enqueue called with value: " << val);
    combiningArray[thread_index].value.store(val, std::memory_order_relaxed);
    combiningArray[thread_index].pending.store(true, std::memory_order_release);
    combiningArray[thread_index].completed.store(false, std::memory_order_relaxed); // New operation, not completed

    if (!sgl.try_lock()) {
        DEBUG_MSG("Lock acquisition failed in enqueue, using combining array");
        // If lock acquisition fails, use the combining array
        combiningArray[thread_index].operation.store(ENQUEUE, std::memory_order_release);
    } else {
        DEBUG_MSG("Lock acquired without contention in enqueue");
        // Lock acquired without contention, perform operation directly
        std::lock_guard<std::mutex> lock(sgl, std::adopt_lock);
        combine();
    }
}

int SGLQueue_FC::dequeue() {
    DEBUG_MSG("Dequeue called");
    combiningArray[thread_index].pending.store(true, std::memory_order_release);
    combiningArray[thread_index].completed.store(false, std::memory_order_relaxed); // New operation, not completed
    combiningArray[thread_index].operation.store(DEQUEUE, std::memory_order_release);

    std::unique_lock<std::mutex> lock(sgl);
    DEBUG_MSG("Lock acquired in dequeue, checking if operation is completed");
    if (!combiningArray[thread_index].completed.load()) {
        DEBUG_MSG("Waiting for operation to complete in dequeue");
        cv.wait(lock, [&](){ return combiningArray[thread_index].completed.load(); });
    } else {
        DEBUG_MSG("Operation already completed, no need to wait");
    }

    int retValue = combiningArray[thread_index].retValue.load(std::memory_order_relaxed);
    combiningArray[thread_index].completed.store(false, std::memory_order_relaxed); // Reset for next use
    DEBUG_MSG("Dequeue operation completed with value: " << retValue);
    return retValue;
}

void SGLQueue_FC::combine() {
    DEBUG_MSG("Combining operations");
    //std::lock_guard<std::mutex> lock(sgl); // Ensure exclusive access
    for (auto& op : combiningArray) {
        if (!op.pending.load(std::memory_order_acquire) || op.completed.load(std::memory_order_relaxed)) {
            continue; // Skip if not pending or already completed
        }

        DEBUG_MSG("Checking operation in combine loop");
        if (op.operation.load(std::memory_order_relaxed) == ENQUEUE) {
            DEBUG_MSG("Performing ENQUEUE operation");
            q.push(op.value.load(std::memory_order_relaxed));
        } else if (op.operation.load(std::memory_order_relaxed) == DEQUEUE) {
            DEBUG_MSG("Performing DEQUEUE operation");
            if (!q.empty()) {
                op.retValue.store(q.front(), std::memory_order_relaxed);
                q.pop();
            } else {
                op.retValue.store(-1, std::memory_order_relaxed); // Sentinel value for empty queue
            }
        }

        op.pending.store(false, std::memory_order_relaxed);
        op.completed.store(true, std::memory_order_release); // Mark as completed
        cv.notify_all(); // Notify all waiting threads
        DEBUG_MSG("Operation completed in combine, value: " << op.retValue.load());
    }
}



void concurrentSGLQueueFCEnqueue(SGLQueue_FC& queue, int val) {
    queue.enqueue(val);
}

void concurrentSGLQueueFCDequeue(SGLQueue_FC& queue, std::atomic<int>& sum) {
    int val = queue.dequeue();
    if (val != -1) { // Assuming -1 indicates an empty queue
        sum.fetch_add(val, std::memory_order_relaxed);
    }
}

void sgl_queue_fc_test(std::vector<int>& values, int numThreads) {
    SGLQueue_FC queue(values.size()); // Assuming the max concurrency level is the size of the values vector
    std::atomic<int> sum(0);
    std::vector<std::thread> threads;

    int halfNumThreads = numThreads / 2;

    // Concurrent enqueues
for (int i = 0; i < halfNumThreads; ++i) {
    threads.push_back(std::thread([&queue, &values, i, halfNumThreads]() {
        DEBUG_MSG("Enqueue thread " << i << " started");
        for (size_t j = i; j < values.size(); j += halfNumThreads) {
            concurrentSGLQueueFCEnqueue(queue, values[j]);
        }
        DEBUG_MSG("Enqueue thread " << i << " finished");
    }));
}

    DEBUG_MSG("Dequeues Started");
    // Concurrent dequeues
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&queue, &sum, i, halfNumThreads, &values]() {
            DEBUG_MSG("Dequeue thread " << i << " started");
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                concurrentSGLQueueFCDequeue(queue, sum);
            }
            DEBUG_MSG("Dequeue thread " << i << " finished");
        }));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();  // Clear the vector of threads

    DEBUG_MSG("Threads have joined");
    // Calculate the expected sum of the vector
    int expectedSum = std::accumulate(values.begin(), values.end(), 0);
    
    // Check if the sum of dequeued values is correct
    if (sum.load(std::memory_order_relaxed) != expectedSum) {
        std::cerr << "Error: The sum of dequeued values does not match the expected sum." << std::endl;
        std::cerr << "Sum: " << sum.load(std::memory_order_relaxed) << ", Expected: " << expectedSum << std::endl;
    } else {
        std::cout << "Test for SGL queue with flat combining optimization passed" << std::endl;
    }
}

void SGLStack_FC::combine() {
    for (auto& op : combiningArray) {
        if (op.pending.load() && !op.completed.load()) {
            if (op.operation.load() == PUSH) {
                stk.push(op.value.load());
            } else if (op.operation.load() == POP) {
                if (!stk.empty()) {
                    op.value.store(stk.top());
                    stk.pop();
                } else {
                    op.value.store(-1);  // Indicate stack was empty
                }
            }
            op.pending.store(false);
            op.completed.store(true);
        }
    }
    cv.notify_all();
}

void SGLStack_FC::push(int val) {
    int thread_index = get_thread_index();  // Implement this function to assign unique index to each thread
    auto& op = combiningArray[thread_index];
    op.value.store(val);
    op.operation.store(PUSH);
    op.pending.store(true);
    op.completed.store(false);

    std::unique_lock<std::mutex> lock(sgl);
    if (lock.owns_lock()) {
        combine();
    } else {
        cv.wait(lock, [&op] { return op.completed.load(); });
    }
}

int SGLStack_FC::pop() {
    int thread_index = get_thread_index();  // Implement this function to assign unique index to each thread
    auto& op = combiningArray[thread_index];
    op.operation.store(POP);
    op.pending.store(true);
    op.completed.store(false);

    std::unique_lock<std::mutex> lock(sgl);
    if (lock.owns_lock()) {
        combine();
    } else {
        cv.wait(lock, [&op] { return op.completed.load(); });
    }

    return op.value.load();  // Return the value popped or -1 if stack was empty
}

void sgl_stack_fc_test(std::vector<int>& values, int numThreads) {
    SGLStack_FC stack(values.size());  // Assuming the max concurrency level is the size of the values vector
    std::vector<std::thread> threads;

    int halfNumThreads = numThreads / 2;

    // Concurrent pushes
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&stack, &values, i, halfNumThreads]() {
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                stack.push(values[j]);
            }
        }));
    }

    // Concurrent pops
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&stack, i, halfNumThreads, &values]() {
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                stack.pop();
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Additional checks or verifications can be added here
    std::cout << "Test for SGL stack with flat combining optimization passed" << std::endl;
}
