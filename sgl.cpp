/********************************************************************
 * @author Suraj Ajjampur
 * @file sgl.cpp
 * 
 * @brief This source file implements a Stack and Queue to be used in a
 * multithreaded application using a Single-Global Lock.
 * 
 * @date 14 Dec 2023
********************************************************************/

#include "sgl.h"


void SGLQueue::enqueue(int val) {
    std::lock_guard<std::mutex> lock(sgl);
    q.push_back(val);
}

int SGLQueue::dequeue() {
    std::lock_guard<std::mutex> lock(sgl);
    if (q.empty()) {
        // Handle empty queue, e.g., throw an exception or return a special value
        return -1;  // Example: return -1 to indicate an empty queue
    }
    int ret = q.front();
    q.pop_front();
    return ret;
}

void concurrentSGLQueueEnqueue(SGLQueue& queue, int val) {
    queue.enqueue(val);
}

void concurrentSGLQueueDequeue(SGLQueue& queue, std::atomic<int>& sum) {
    int val = queue.dequeue();
    if (val != -1) { // Assuming -1 indicates an empty queue
        sum.fetch_add(val, RELAXED);
    }
}

void testConcurrentSGLQueueOperations() {
    SGLQueue queue;
    std::atomic<int> sum(0);
    std::vector<std::thread> threads;

    // Start threads to perform concurrent enqueues
    for (int i = 1; i <= 5; ++i) {
        threads.push_back(std::thread(concurrentSGLQueueEnqueue, std::ref(queue), i));
    }

    // Start threads to perform concurrent dequeues
    for (int i = 0; i < 5; ++i) {
        threads.push_back(std::thread(concurrentSGLQueueDequeue, std::ref(queue), std::ref(sum)));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Check if the sum of dequeued values is correct
    assert(sum == 15); // 1+2+3+4+5 = 15

    std::cout << "Test Concurrent SGL Queue Operations: Passed" << std::endl;
}

void sgl_queue_test(std::vector<int>& values, int numThreads) {
    SGLQueue queue;
    std::atomic<int> sum(0);
    std::vector<std::thread> threads;

    int halfNumThreads = numThreads / 2;

    // Concurrent enqueues
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&queue, &values, i, halfNumThreads]() {
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                concurrentSGLQueueEnqueue(queue, values[j]);
            }
        }));
    }

    // Concurrent dequeues
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&queue, &sum, i, halfNumThreads, &values]() {
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                concurrentSGLQueueDequeue(queue, sum);
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Calculate the expected sum of the vector
    int expectedSum = std::accumulate(values.begin(), values.end(), 0);

    // Check if the sum of dequeued values is correct
    if (sum != expectedSum) {
        std::cerr << "Error: The sum of dequeued values does not match the expected sum." << std::endl;
        std::cerr << "Sum: " << sum << ", Expected: " << expectedSum << std::endl;
    } else {
        std::cout << "Test for SGL queue passed with no optimization" << std::endl;
    }
}



void testBasicSGLQueueOperations() {
    SGLQueue queue;

    // Enqueue elements
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    // Dequeue and check elements
    assert(queue.dequeue() == 1);
    assert(queue.dequeue() == 2);
    assert(queue.dequeue() == 3);

    // Queue should be empty now
    assert(queue.dequeue() == -1); // Assuming -1 indicates an empty queue

    std::cout << "Test Basic SGL Queue Operations: Passed" << std::endl;
}





void SGLStack::push(int val) {
    std::lock_guard<std::mutex> lock(sgl);
    q.push_back(val);
}

int SGLStack::pop() {
    std::lock_guard<std::mutex> lock(sgl);
    if (q.empty()) {
        return -1;  //return -1 to indicate an empty queue
    }
    int ret = q.back();
    q.pop_back();
    return ret;
}

void concurrentSGLStackPush(SGLStack& stack, int val) {
    stack.push(val);
}

void concurrentSGLStackPop(SGLStack& stack, std::atomic<int>& popCount) {
    if (stack.pop() != -1) {
        popCount.fetch_add(1, RELAXED);
    }
}

void testConcurrentSGLStackOperations() {
    SGLStack stack;
    std::atomic<int> popCount(0);
    std::vector<std::thread> threads;

    // Start threads to perform concurrent pushes
    for (int i = 0; i < 100; ++i) {
        threads.push_back(std::thread(concurrentSGLStackPush, std::ref(stack), i));
    }

    // Start threads to perform concurrent pops
    for (int i = 0; i < 100; ++i) {
        threads.push_back(std::thread(concurrentSGLStackPop, std::ref(stack), std::ref(popCount)));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Check if the number of successful pops matches the number of pushes
    assert(popCount == 100);

    std::cout << "Test Concurrent SGL Stack Operations: Passed" << std::endl;
}

void sgl_stack_test(std::vector<int>& values, int numThreads) {
    SGLStack stack;
    std::atomic<int> popCount(0);
    std::vector<std::thread> threads;

    int halfNumThreads = numThreads / 2;

    // Concurrent pushes
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&stack, &values, i, halfNumThreads]() {
            for (int j = i; j < values.size(); j += halfNumThreads) {
                concurrentSGLStackPush(stack, values[j]);
            }
        }));
    }

    #ifdef DEBUG_MODE
    DEBUG_MSG("Begin Pop");
    #endif

    // Concurrent pops
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&stack, &popCount, i, halfNumThreads, &values]() {
            for (int j = i; j < values.size(); j += halfNumThreads) {
                concurrentSGLStackPop(stack, popCount);
            }
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    if (popCount.load(RELAXED) != values.size()) {
        std::cerr << "Error: The number of successful pops does not match the number of pushes." << std::endl;
        std::cerr << "Pops: " << popCount << ", Pushes: " << values.size() << std::endl;
    } else {
        std::cout << "Test for SGL stack passed with no optimization" << std::endl;
    }
}




