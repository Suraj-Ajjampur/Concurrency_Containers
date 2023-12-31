/*****************************************************************
 * @author Suraj Ajjampur
 * @file   elimination.h
 * 
 * @brief This C++ header file implements Trieber stack using the 
 *        elimination method in order to deal with contention issues.
 * 
 * @date 15 Dec 2023
********************************************************************/

#ifndef ELIMINATION_H
#define ELIMINATION_H

#include "my_atomics.h"
#include <cstddef>  // for std::uintptr_t
#include <assert.h>
#include <atomic>
#include <vector>
#include <random> // For random number generation
#include <chrono>
#include <thread>
#include <list>

class tstack_e {
public:
    struct node {
        std::atomic<int> val;    // Data variable of the node
        std::atomic<node*> down; // This is like a next pointer of a node in LIFO
        node(int v) : val(v), down(nullptr) {} // Constructor of node
    };

    struct EliminationSlot {
        std::atomic<int> value;
        std::atomic<bool> active;
        std::atomic<bool> isPush; // True for push, false for pop

        EliminationSlot() : value(0), active(false), isPush(false) {}
    };

    class EliminationArray {
    public:
        std::vector<EliminationSlot> slots;
        const int size;

        EliminationArray(int size) : size(size), slots(size) {}
    };

    atomic<node*> top; // Now an atomic pointer, not just a pointer to node
    EliminationArray eliminationArray;

    tstack_e(int eliminationSize) : eliminationArray(eliminationSize) {} // Constructor

    void push(int val); 
    int pop();
    bool tryElimination(int& val, bool isPush);
    
    // Random number generator for choosing a slot in the elimination array
    std::mt19937 rng{std::random_device{}()};
    int getRandomSlotIndex() {
        std::uniform_int_distribution<int> dist(0, eliminationArray.size - 1);
        return dist(rng);
    }
};

class SGLStack_e {
private:
    std::mutex sgl;
    std::list<int> q;

    struct EliminationSlot {
        std::atomic<int> value;
        std::atomic<bool> active;
        std::atomic<bool> isPush;

        EliminationSlot() : value(0), active(false), isPush(false) {}
    };

    class EliminationArray {
    public:
        std::vector<EliminationSlot> slots;
        const int size;
        std::mt19937 rng; // Random number generator

        EliminationArray(int size) : size(size), slots(size), rng(std::random_device{}()) {}

        int getRandomSlotIndex() {
            std::uniform_int_distribution<int> dist(0, size - 1);
            return dist(rng);
        }
    };

    EliminationArray eliminationArray;

public:
    SGLStack_e(int eliminationSize) : eliminationArray(eliminationSize) {}

    void push(int val);
    int pop();
    bool tryElimination(int& val, bool isPush);
};

void test_ts_elimination(void);
void treiber_stack_elimination_test(std::vector<int>& values, int numThreads);
void sgl_stack_elimination_test(std::vector<int>& values, int numThreads);

#endif //ELIMINATION_H