/*****************************************************************
 * @author Suraj Ajjampur
 * @file   elimination.cpp
 * 
 * @brief This C++ source file implements stacks using the 
 *        elimination method in order to deal with contention issues.
 * 
 * @date 15 Dec 2023
********************************************************************/

#include "elimination.h"
#define ELIMINATION_ARRAY_SIZE 5

bool tstack_e::tryElimination(int& val, bool isPush) {
    // Select a random slot in the elimination array
    int slotIndex = getRandomSlotIndex();
    EliminationSlot& slot = eliminationArray.slots[slotIndex];

    if (!slot.active.load(ACQUIRE)) {
        // Slot is empty, try to place operation there
        int expected = 0;
        if (cas(slot.value,expected, val, ACQ_REL)) {
            slot.isPush.store(isPush, RELEASE);
            slot.active.store(true, RELEASE);

            // Wait for a short duration for a match
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // You can adjust this duration

            if (slot.active.load(ACQUIRE)) {
                // No operation combined, remove the operation
                slot.active.store(false, RELEASE);
                return false; // Failed to eliminate, need to retry stack operation
            } else {
                // Operation combined
                if (!isPush) {
                    val = slot.value.load(RELAXED); // For pop, update the value
                }
                return true;
            }
        }
    } else if (slot.isPush.load(ACQUIRE) != isPush) {
        // Found an opposite operation, try to combine
        int oppositeValue = slot.value.load(ACQUIRE);
        if (cas(slot.active,true, false, ACQ_REL)) {
            // Successfully exchanged
            if (!isPush) {
                val = oppositeValue; // For pop, update the value to the pushed value
            }
            return true;
        }
    }

    // Slot unusable or operation not combined, return false
    return false;
}


void tstack_e::push(int val) {
    node* n = new node(val);
    node* old_top;
    while (true) {
        old_top = top.load(ACQUIRE);
        n->down.store(old_top, RELAXED);
        if (cas(top, old_top, n, ACQ_REL)) {
            break; // Successfully pushed
        } else {
            // Attempt to use the elimination array to relieve contention
            if (!tryElimination(val, true)) {
                continue; // Retry stack operation if elimination fails
            }
            break; // Successfully exchanged in elimination array
        }
    }
}

int tstack_e::pop() {
    while (true) {
        node* t = top.load(ACQUIRE);
        if (t == nullptr) {
            return -1; // Stack is empty
        }

        node* n = t->down.load(RELAXED);
        int v = t->val.load(RELAXED);

        if (cas(top, t, n, ACQ_REL)) {
            //delete t; // Successfully popped
            return v;
        } else {
            // Attempt to use the elimination array
            int result;
            if (!tryElimination(result, false)) {
                continue; // Retry stack operation if elimination fails
            }
            return result; // Successfully exchanged in elimination array
        }
    }
}

void thread_function(tstack_e& stack, bool isPushThread, int numOps) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1000);

    for (int i = 0; i < numOps; i++) {
        if (isPushThread) {
            stack.push(dis(gen)); // Push random value
        } else {
            stack.pop(); // Pop value
        }
    }
}


// Test function for
void test_ts_elimination(void){

    // Specifying the size of the elimination array to 10
    tstack_e stack(12);
    const int numThreads = 128;
    const int numOps = 10000; // Number of operations per thread

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    // Creating numOps threads to alternate between push and pop operation 
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(thread_function, std::ref(stack), i % 2 == 0, numOps);
    }

    // Joining threads
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Total time taken: " << diff.count() << " s" << std::endl;

}


void Push(tstack_e& stack, int val) {
    stack.push(val);
}

void Pop(tstack_e& stack, std::atomic<int>& popCount) {
    int val = stack.pop();
    if ( val != -1) {
        popCount.fetch_add(1, RELAXED);
    }else{cout <<"Stack is empty Error" << endl;}
}

void treiber_stack_elimination_test(std::vector<int>& values, int numThreads) {
    tstack_e stack(ELIMINATION_ARRAY_SIZE);
    std::atomic<int> popCount(0);
    std::vector<std::thread> threads;

    // Half the threads for pushing, half for popping
    int halfNumThreads = numThreads / 2;

    // Create threads to perform concurrent pushes
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&stack, &values, i, halfNumThreads]() {
            for (int j = i; j < values.size(); j += halfNumThreads) {
                Push(stack, values[j]);
            }
        }));
    }

    DEBUG_MSG("Begin Pop");
    // Create threads to perform concurrent pops
    for (int i = 0; i < values.size(); ++i) {
        threads.push_back(std::thread(Pop, std::ref(stack), std::ref(popCount)));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Check if the number of successful pops matches the number of pushes
    if (popCount.load(RELAXED) != values.size()) {
        std::cerr << "Error: The number of successful pops does not match the number of pushes." << std::endl;
        std::cerr << "Pops: " << popCount << ", Pushes: " << values.size() << std::endl;
        // Handle error
    } else {
        std::cout << "Test for Treiber stack passed with Elimination optimization" << std::endl;
    }
}


void SGLStack_e::push(int val) {
    if (!sgl.try_lock()) {
        if (!tryElimination(val, true)) {
            std::lock_guard<std::mutex> lock(sgl);
            q.push_back(val);
        }
    } else {
        std::lock_guard<std::mutex> lock(sgl, std::adopt_lock);
        q.push_back(val);
    }
}

int SGLStack_e::pop() {
    if (!sgl.try_lock()) {
        int val;
        if (!tryElimination(val, false)) {
            std::lock_guard<std::mutex> lock(sgl);
            if (q.empty()) return -1;
            int ret = q.back();
            q.pop_back();
            return ret;
        } else {
            return val;
        }
    } else {
        std::lock_guard<std::mutex> lock(sgl, std::adopt_lock);
        if (q.empty()) return -1;
        int ret = q.back();
        q.pop_back();
        return ret;
    }
}

bool SGLStack_e::tryElimination(int& val, bool isPush) {
    // Ensure the random slot selection function is defined
    int slotIndex = eliminationArray.getRandomSlotIndex();
    EliminationSlot& slot = eliminationArray.slots[slotIndex];

    if (!slot.active.load(ACQUIRE)) {
        int expected = 0;
        if (std::atomic_compare_exchange_strong(&slot.value, &expected, val)) {
            slot.isPush.store(isPush, RELEASE);
            slot.active.store(true, RELEASE);

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Adjustable duration

            if (slot.active.load(ACQUIRE)) {
                slot.active.store(false, RELEASE);
                return false; // Failed to eliminate, retry stack operation
            } else {
                if (!isPush) {
                    val = slot.value.load(RELAXED); // For pop, update the value
                }
                return true; // Operation combined
            }
        }
    } else if (slot.isPush.load(ACQUIRE) != isPush) {
        bool expected = true;
        if (cas(slot.active, expected, false, ACQ_REL)) {
            if (!isPush) {
                val = slot.value.load(ACQUIRE);
            }
            return true; // Successfully exchanged
        }
    }

    return false; // Slot unusable or operation not combined
}

void concurrentSGLStackPush(SGLStack_e& stack, int val) {
    stack.push(val);
}

void concurrentSGLStackPop(SGLStack_e& stack, std::atomic<int>& popCount) {
    if (stack.pop() != -1) {
        popCount.fetch_add(1, RELAXED);
    }
}

void sgl_stack_elimination_test(std::vector<int>& values, int numThreads) {
    SGLStack_e stack(ELIMINATION_ARRAY_SIZE);
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
        std::cout << "Test for SGL stack passed with Elimination optimization" << std::endl;
    }
}
