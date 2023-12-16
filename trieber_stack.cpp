/*****************************************************************
 * @author Suraj Ajjampur
 * @file   trieber_stack.cpp
 * 
 * @brief This C++ source file implements Trieber stack which is a 
 * non-blocking data structure. It is linearizable and lock-free
 * 
 * Garbage Collection Issues currently dealt with 
 * 1) Race against reclaimation - Done 
 * 2) ABA problem - To-do
 * 
 * @date 14 Dec 2023
********************************************************************/

#include <cstddef>  // for std::uintptr_t
#include "trieber_stack.h"

#define CONTENTION_OPT 1

/** Pushes the value onto the stack
 * 
 * @param val integer to be added onto the stack
 */
void tstack::push(int val){
    // Creating a new node and attempting to push it onto the stack
    node* n = new node(val);
    node* old_top;
    do {
        old_top = top.load(ACQUIRE); // Load the current value of top
        n->down.store(old_top, RELAXED); // Set the new node's next pointer to the current top
        // Attempt to swap the old top with the new top.
        // If another thread has modified the top, the CAS will fail and retry.
#if CONTENTION_OPT == 0
    } while(!cas(top, old_top, n, ACQ_REL)); // This is the linearization point of push
#else
    } while(top.load(ACQUIRE) == old_top && !cas(top, old_top, n, ACQ_REL));
#endif   
}


/** Returns the most recent pushed value - recent is defined by linearizability
 * 
 * @return The value of the popped node, or NULL if the stack is empty.
 */
int tstack::pop(void){
    node* t;
    node* n;
    int v;
    do {
        t = top.load(ACQUIRE); // Load the current value of top

        if(t == nullptr){ 
            return -1; // Stack is empty, return NULL
        }
        n = t->down.load(RELAXED); // Get the next node
        v = t->val.load(RELAXED); // Read the value from the current top node
        // Attempt to swap the old top with the new top.
        // If another thread has modified the top, the CAS will fail and retry.
#if CONTENTION_OPT == 0
    } while(!cas(top, t, n, ACQ_REL)); // This is the linearization point of pop
#else 
    } while(top.load(ACQUIRE) == t && !cas(top, t, n, ACQ_REL));
#endif
    // Memory reclamation should be performed here.
    delete t;
    // Delete the old node after ensuring no other threads are accessing it.
    return v; // Return the value of the popped node
}

void push3_pop_till_empty(void){
    /************ Testing for the Trieber Stack here **********/
    tstack stack; // Create an instance of tstack

    // Test the push function
    std::cout << "Pushing values onto the stack..." << std::endl;
    stack.push(1);
    stack.push(2);
    stack.push(3);

    // Test the pop function
    std::cout << "Popping values from the stack till empty..." << std::endl;
    int val;
    while ((val = stack.pop()) != -1) { // Continue popping until the stack is empty
        std::cout << "Popped: " << val << std::endl;
    }
}

void push_pop(void){
    /************ Testing for the Trieber Stack here **********/
    tstack stack; // Create an instance of tstack

    // Test the push function
    std::cout << "Pushing then popping alternatively" << std::endl;
    stack.push(1);
    cout << "Value is " << stack.pop() << endl;
    stack.push(2);
    cout << "Value is " << stack.pop() << endl;
    stack.push(3);
    cout << "Value is " << stack.pop() << endl;
}

void concurrentPush(tstack& stack, int val) {
    stack.push(val);
}

void concurrentPop(tstack& stack, std::atomic<int>& popCount) {
    if (stack.pop() != -1) {
        popCount.fetch_add(1, std::memory_order_relaxed);
    }
}

void testConcurrentPushPop() {
    tstack stack;
    const int numOperations = 100;
    std::atomic<int> popCount(0);
    std::vector<std::thread> threads;

    // Create threads to perform concurrent pushes
    for (int i = 0; i < numOperations; ++i) {
        threads.push_back(std::thread(concurrentPush, std::ref(stack), i));
    }

    // Create threads to perform concurrent pops
    for (int i = 0; i < numOperations; ++i) {
        threads.push_back(std::thread(concurrentPop, std::ref(stack), std::ref(popCount)));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Check if the number of successful pops matches the number of pushes
    assert(popCount == numOperations);

    std::cout << "Test Concurrent Push Pop: Passed" << std::endl;
}
