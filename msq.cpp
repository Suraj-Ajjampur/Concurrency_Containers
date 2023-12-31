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
#include <numeric>

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
    DEBUG_MSG("Enqued value is " << val);
}

void concurrentDequeue(msqueue& queue, std::atomic<int>& sum) {
    int val = queue.dequeue();

    if (val != -1) {
        DEBUG_MSG("dequed value is " << val);
        sum.fetch_add(val, SEQ_CST);
    }
}

/**
 * @brief Tests the M&S lock-free queue with a given set of values and a specified number of threads.
 * 
 * This function performs concurrent enqueues and dequeues on an M&S queue using multiple threads. The number of
 * threads is divided equally between enqueue and dequeue operations. It validates the test by comparing the sum
 * of dequeued values against the expected sum calculated from the input values.
 * 
 * @param values A vector of integers to be enqueued into the queue.
 * @param numThreads The total number of threads to be used for concurrent enqueue and dequeue operations.
 * 
 * @note The function asserts if the sum of dequeued values does not match the expected sum, indicating an issue
 *       with the queue's concurrent operation handling.
 */
void ms_queue_test(std::vector<int>& values, int numThreads) {
    msqueue queue;
    std::atomic<int> sum(0);
    std::vector<std::thread> threads;

    int halfNumThreads = numThreads / 2;

    // Concurrent enqueues
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&queue, &values, i, halfNumThreads]() {
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                concurrentEnqueue(queue, values[j]);
            }
        }));
    }

    // Concurrent dequeues
    for (int i = 0; i < halfNumThreads; ++i) {
        threads.push_back(std::thread([&queue, &sum, i, halfNumThreads, &values]() {
            for (size_t j = i; j < values.size(); j += halfNumThreads) {
                concurrentDequeue(queue, sum);
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
        std::cout << "Test for M&S queue passed !" << std::endl;
    }
}
