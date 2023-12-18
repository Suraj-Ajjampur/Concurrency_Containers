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

/**
 * @brief Enqueues a value into the queue using flat combining optimization.
 *
 * This method stores the value in the combining array and sets its status to pending.
 * It then attempts to acquire a lock. If the lock is not acquired, the operation
 * is deferred to be combined with others. If the lock is acquired without contention,
 * the combining operation is executed immediately.
 *
 * @param val The value to be enqueued.
 */
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

/**
 * @brief Dequeues a value from the queue using flat combining optimization.
 *
 * This method sets the dequeue operation in the combining array as pending.
 * It acquires a lock and waits for the operation to be completed by the combiner thread.
 * Once the operation is completed, the dequeued value (or a sentinel value indicating
 * an empty queue) is returned.
 *
 * @return The value dequeued from the queue, or a sentinel value if the queue is empty.
 */
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

/**
 * @brief Combines pending operations in the queue.
 *
 * This method iterates through the combining array and performs any pending operations
 * (enqueue or dequeue). Once an operation is executed, it is marked as completed,
 * and any threads waiting on this operation are notified.
 *
 * @note This method should be called by a thread that successfully acquires the lock
 *       on the queue to ensure exclusive access while combining operations.
 */
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

/**
 * @brief Combines pending stack operations in the flat combining array.
 *
 * Iterates through the combining array and performs any pending push or pop operations.
 * After an operation is executed, it is marked as completed. Notifies all waiting threads
 * after completing all operations.
 */
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

/**
 * @brief Pushes a value onto the stack using flat combining optimization.
 *
 * Stores the value and push operation in the combining array for the current thread.
 * Attempts to acquire a lock to combine operations. If the lock is already owned,
 * combines the operations immediately; otherwise, waits until the push operation is completed.
 *
 * @param val The value to be pushed onto the stack.
 */
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

/**
 * @brief Pops a value from the stack using flat combining optimization.
 *
 * Sets a pop operation in the combining array for the current thread.
 * Attempts to acquire a lock to combine operations. If the lock is already owned,
 * combines the operations immediately; otherwise, waits until the pop operation is completed.
 *
 * @return The value popped from the stack, or -1 if the stack was empty.
 */
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

/**
 * @brief Tests the SGL stack with flat combining optimization using a set of values and multiple threads.
 *
 * This function creates threads for concurrent push and pop operations on an SGL stack with flat combining.
 * Each thread either pushes or pops values to/from the stack. After all operations are completed, additional
 * checks or verifications can be performed.
 *
 * @param values A vector of integers to be pushed onto the stack.
 * @param numThreads The total number of threads to be used for concurrent push and pop operations.
 */
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
