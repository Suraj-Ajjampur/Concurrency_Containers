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
        class node{
            public:
                atomic<int> val;    // Data variable of the node
                atomic<node*> down; //This is like a next pointer of a node in lifo
                node(int v) :val(v){} //Constructor of node
        };
        
        atomic<node*> top; // Pointer to the top of the stack
        void push(int val); 
        int pop();
};

/** Pushes the value onto the stack
 * 
 * @param val integer to be added onto the stack
 */
void tstack::push(int val){
    //Creating a new node and pushing it on top of the stack
    node* n = new node(val);
    node* t;
    do{

        t = top.load(ACQUIRE);
        n->down = t; //Read the current value of top and assign 

    }while(!cas(top,t,n,ACQ_REL)); // This is the linearization point of push 
    // Then we are going to attempt a compare and swap,
    // If this while succeeded, we succeeded in pushing the top of the value
    // If we failed that means, another node pushed or popped top -- basically modified top
}

/** Returns the most recent push - recent is defined by linearizability
 * 
 */
int tstack::pop(void){
    node* t;
    node* n;
    int v;
    do{
        t = top.load(ACQUIRE); // load value of top to t

        if(t==NULL){return NULL;} //Check if the stack is empty
        n = t->down; //assign 
        v = t->val; // Get the value of the top node

    }while(!cas(top,t,n,ACQ_REL));  // This is the linearization point of push 
    // Then we are going to attempt a compare and swap,
    // If this while succeeded, we succeeded in pushing the top of the value
    // If we failed that means, another node pushed or popped top -- basically modified top
    return v;
}
