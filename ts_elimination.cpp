/*****************************************************************
 * @author Suraj Ajjampur
 * @file   ts_elimination.cpp
 * 
 * @brief This C++ source file implements Trieber stack using the 
 *        elimination method in order to deal with contention issues.
 * 
 * @date 15 Dec 2023
********************************************************************/

#include "ts_elimination.h"


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

            if (slot.active.load(std::memory_order_acquire)) {
                // No operation combined, remove the operation
                slot.active.store(false, std::memory_order_release);
                return false; // Failed to eliminate, need to retry stack operation
            } else {
                // Operation combined
                if (!isPush) {
                    val = slot.value.load(std::memory_order_relaxed); // For pop, update the value
                }
                return true;
            }
        }
    } else if (slot.isPush.load(std::memory_order_acquire) != isPush) {
        // Found an opposite operation, try to combine
        int oppositeValue = slot.value.load(std::memory_order_acquire);
        if (cas(slot.active,true, false, std::memory_order_acq_rel)) {
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
        old_top = top.load(std::memory_order_acquire);
        n->down.store(old_top, std::memory_order_relaxed);
        if (cas(top, old_top, n, std::memory_order_acq_rel)) {
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
        node* t = top.load(std::memory_order_acquire);
        if (t == nullptr) {
            return -1; // Stack is empty
        }

        node* n = t->down.load(std::memory_order_relaxed);
        int v = t->val.load(std::memory_order_relaxed);

        if (cas(top, t, n, std::memory_order_acq_rel)) {
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