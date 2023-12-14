#ifndef TRIEBER_STACK_H
#define TRIEBER_STACK_H

#include "my_atomics.h"
#include <cstddef>  // for std::uintptr_t

/** Nonblocking data structure which is linearizable and Lock-free
 * 
 */
class tstack{
public:
    struct node{
        atomic<int> val;    // Data variable of the node
        atomic<node*> down; // This is like a next pointer of a node in LIFO
        node(int v) : val(v), down(nullptr) {} // Constructor of node
    };
    
    // This struct will hold both the pointer to the top node and the ABA counter.
    struct alignas(16) cnt_ptr {
        node* ptr;
        unsigned long count;
    };
    
    atomic<cnt_ptr> top; // Now an atomic cnt_ptr, not just a pointer to node

    void push(int val); 
    int pop();
};

#endif