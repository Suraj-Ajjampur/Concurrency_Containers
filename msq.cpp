/*****************************************************************
 * @author Suraj Ajjampur
 * @file msq.cpp
 * 
 * @brief This C++ source file implements the Micheal & Scott Queue, which is a non-blocking linearizable queue 
 *        which enqueues from the tail and dequeues from the head
 * 
 * @date 5 Dec 2023
********************************************************************/

#include "msq.h"
#include <mutex>

mutex lock;

#define CONTENTION_OPT 1

// Constructor
msqueue::msqueue() {
    // We initialize a node pointing to dummy
    node* dummy = new node(DUMMY);
    // We have head and tail point to dummy
    head.store(dummy);
    tail.store(dummy);
}

/** 
 * Make sure tail is up-to-date

 * Add new node (linearization point)
 * Update the 
 * 
 * @param val to be enqueued
 */ 
void msqueue::enqueue(int val){

    // t is a copy of the tail pointer
    // e is the expected value of next pointer of the current tail
    // n is the new node
    node *t, *expected_val_tail_next, *new_node;
    new_node = new node(val);
    while(true){
    // Read both tail and what tail points
    t = tail.load(ACQUIRE); 
    expected_val_tail_next = t->next.load(ACQUIRE);
    //Validate nothing has changed 
    if(t == tail.load()){
        //Step 2: Add new node (linearization point) and update
        node* expected_copy = NULL;
        if(expected_val_tail_next==NULL && cas(t->next,expected_copy,new_node,ACQ_REL)){break;}
        //Step 1: Update the tail we are looking to enqueue to, and retry
        else if(expected_val_tail_next!=NULL){cas(tail,t,expected_val_tail_next,ACQ_REL);} 
    } 
    }
    //Step 3: update the tail -- doesn't matter if this failed
    cas(tail,t,new_node,ACQ_REL);
}

/** 
 * @return val of node which is currently at the real head
 */
int msqueue::dequeue(){
    //h should be made into a counted pointer
    node *t, *h, *n; 
    while(true){
        //Step 1:Snapshot head, tail and dummy
        h=head.load(ACQUIRE); t=tail.load(ACQUIRE); n=h->next.load(ACQUIRE);
        if(h==head.load(ACQUIRE)){
            //Check of the list is empty
            if(h==t){
                //Step 2 if empty, return
                if(n==NULL){return -1;}
                //Else cas tail to be head's next node
                else{cas(tail,t,n,ACQ_REL);}
            }
            //Step 2 Else upate head (linearization point, old head becomes dummy)
            else{
            int ret=n->val;
            #if CONTENTION_OPT == 0
            if(cas(head,h,n,ACQ_REL)){return ret;}
            #else
            if(head.load(ACQUIRE) == h && cas(head,h,n,ACQ_REL)){return ret;}
            #endif
            }
        }
    }
}

void testBasicQueueOperations() {
    msqueue queue;

    // Enqueue elements
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    // Dequeue and check elements
    assert(queue.dequeue() == 1);
    assert(queue.dequeue() == 2);
    assert(queue.dequeue() == 3);

    // Queue should be empty now
    assert(queue.dequeue() == -1);

    std::cout << "Test Basic Queue Operations: Passed" << std::endl;
}

void concurrentEnqueue(msqueue& queue, int val) {
    queue.enqueue(val);
    cout << "Enqued value is " << val << endl;
}

void concurrentDequeue(msqueue& queue, std::atomic<int>& sum) {
    int val = queue.dequeue();

    if (val != -1) {
        cout << "dequed value is " << val << endl;
        sum.fetch_add(val, SEQ_CST);
    }
}

void testConcurrentQueueOperations() {
    msqueue queue;
    std::atomic<int> sum(0);
    std::vector<std::thread> threads;

    // Start threads to perform concurrent enqueues
    for (int i = 1; i <= 100; i++) {
        threads.push_back(std::thread(concurrentEnqueue, std::ref(queue), i));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();  // Clear the vector of threads

    // Start threads to perform concurrent dequeues
    for (int i = 0; i < 100; i++) {
        threads.push_back(std::thread(concurrentDequeue, std::ref(queue), std::ref(sum)));
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    cout << sum << endl;

    // Check if the sum of dequeued values is correct
    assert(sum == 5050);

    std::cout << "Test Concurrent Queue Operations: Passed" << std::endl;
}
