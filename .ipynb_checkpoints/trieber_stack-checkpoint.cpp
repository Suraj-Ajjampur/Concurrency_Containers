/** 
 * 
 */ 
// Includes

// ABA problem during garbage collection
// If the top


//Races through Reclaimation
//Reusing nodes might cause data races -- When the top->val might be reincarnated as a different type

//WE need to do a type presevind memory allocation

#include<my_atomics.h>
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


/** Pushes the value onto the stack
 * 
 * @param val integer to be added onto the stack
 */
/** Pushes the value onto the stack
 * 
 * @param val integer to be added onto the stack
 */
void tstack::push(int val){
    // Creating a new node and attempting to push it onto the stack
    node* n = new node(val);
    cnt_ptr old_top;
    cnt_ptr new_top;
    do {
        old_top = top.load(std::memory_order_acquire); // Load the current value of top
        n->down.store(old_top.ptr, std::memory_order_relaxed); // Set the new node's next pointer to the current top
        new_top.ptr = n; // Prepare the new top value with the new node
        new_top.count = old_top.count + 1; // Increment the counter to address the ABA problem

        // Attempt to swap the old top with the new top.
        // If another thread has modified the top, the CAS will fail and retry.
    } while(!cas(top, old_top, new_top, std::memory_order_acq_rel)); // This is the linearization point of push
}


/** Returns the most recent push - recent is defined by linearizability
 * 
 */
/** Returns the most recent pushed value - recent is defined by linearizability
 * 
 * @return The value of the popped node, or NULL if the stack is empty.
 */
int tstack::pop(void){
    cnt_ptr old_top;
    cnt_ptr new_top;
    int v;
    do {
        old_top = top.load(std::memory_order_acquire); // Load the current value of top

        if(old_top.ptr == nullptr){ 
            return NULL; // Stack is empty, return NULL
        }
        
        node* next_node = old_top.ptr->down.load(std::memory_order_relaxed); // Get the next node
        v = old_top.ptr->val.load(std::memory_order_relaxed); // Read the value from the current top node
        new_top.ptr = next_node; // Update the new top to the next node
        new_top.count = old_top.count + 1; // Increment the counter to address the ABA problem

        // Attempt to swap the old top with the new top.
        // If another thread has modified the top, the CAS will fail and retry.
    } while(!cas(top, old_top, new_top, std::memory_order_acq_rel)); // This is the linearization point of pop

    // Memory reclamation should be performed here.
    delete old_top
    // Delete the old node after ensuring no other threads are accessing it.
    return v; // Return the value of the popped node
}

