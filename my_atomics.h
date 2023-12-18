/*****************************************************************
 * @author Suraj Ajjampur
 * @file my_atomics.h
 * 
 * @brief This C++ header file defines atomic operation functions and memory orderings
 *        using C++ atomics for test-and-set, fetch-and-increment, compare-and-swap,
 *        and a variant of compare-and-swap with a return value.
 * 
 * @date 24 Oct 2023
 * 
 * @assignment Concurrent Programming Fall 2023 (Final Project)
********************************************************************/

#ifndef MY_ATOMICS_H
#define MY_ATOMICS_H

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <ctime>
#include <mutex>

#define DEBUG_MODE 0

#define DEBUG_MSG(msg) \
    do { \
        if (DEBUG_MODE) { \
            std::cout << msg << std::endl; \
        } \
    } while(0)

/** Memory order macros **/
#define SEQ_CST std::memory_order_seq_cst
#define RELAXED std::memory_order_relaxed
#define ACQUIRE std::memory_order_acquire
#define RELEASE std::memory_order_release
#define ACQ_REL std::memory_order_acq_rel

// memory order 0 = Sequencial Consistency
// memory order 1 = Release consistency
#define RELEASE_CONSISTENCY true
#define SEQ_CONISTENCY false

using namespace std;

void tas_lock(atomic<bool>& x);
void tas_unlock(atomic<bool>& x);

void ttas_lock(atomic<bool>& x);
void ttas_unlock(atomic<bool>& x);

void ticket_lock(atomic<int>& next_num, atomic<int>& now_serving);
void ticket_unlock(atomic<int>& now_serving);

int fai(atomic<int>& x, int amount, std::memory_order MEM);

/** Atomically check if the old value is what you expect, if it is, replace it
 * with a new value and indicate success.
 * 
 * @return Bool to indicate if the expected value is equal to the desired value.
 */ 
template <typename T>
bool cas(atomic<T>& x, T expected, T desired, std::memory_order MEM){
 T expected_ref = expected;
 //compare_exchange_strong changes the value from expected_ref to desired
 return x.compare_exchange_strong(expected_ref, desired, MEM); 
}

template <typename T>
T vcas(atomic<T>& x, T expected, T desired, std::memory_order MEM);

/**
 * @brief SenseBarrier class for synchronization among multiple threads.
 *
 * This class implements a barrier synchronization primitive known as the Sense Barrier.
 * It allows multiple threads to synchronize at a common point before proceeding further.
 */
class SenseBarrier {
public:
    /**
     * @brief Constructs a SenseBarrier with a given number of threads.
     *
     * @param numThreads The total number of threads that will synchronize using this barrier.
     */
    SenseBarrier(int numThreads);
    /**
     * @brief Arrives at the barrier and waits for all other threads to arrive.
     *
     * This function is called by each thread when it reaches the barrier. It ensures that
     * all threads reach the barrier before any of them proceed further.
     */
    void ArriveAndWait();

    void ArriveAndWaitRel();

private:
    atomic<int> cnt;    // Counter to track arrivals, propagated to all threads
    atomic<int> sense;  // Sense barrier, propagated to all threads
    int N;              // Total number of threads
};

/**
 *  @brief Mellor-Crummey and Scott(MSCLock) Lock
 * 
 *   This lock uses a queue for waiting threads.
 *   On arrival, we place a node in the queue by append
 *   It spinds on the node
 *   On releasing the lock, notify the next thread in the queue
 */
class MCSLock{
public:
    class Node{
    public:
        atomic<Node*> next; //Atomic pointer
        atomic<bool> wait;
    };

    atomic<Node*> tail; // Member of MCSLock
    /** 
     * @brief Constructs the MCSLock given a pointer to an atomic value
     * 
     * @param tail 
    */
    MCSLock(atomic<Node*>& tail); 

    void acquire(Node* myNode);

    void release(Node* myNode);
};

/**
 * @class Petersons
 *
 * @brief Implements the Peterson's algorithm for mutual exclusion.
 * 
 * @ref https://www.geeksforgeeks.org/petersons-algorithm-for-mutual-exclusion-set-1/
 */
class Petersons {
public:
    atomic<bool> desires[2]; // Array of atomics indicating whether each thread desires the lock.
    atomic<int> turn; // Turn variable to control access to the critical section.
    /**
     * @brief Constructor to initialize the lock.
     *
     * The constructor resets the desires of both threads to acquire the lock and sets the turn to one of them.
     */
    Petersons(bool memory_order);

    /**
     * @brief Lock the critical section for a thread for seq consistent memory order
     *
     * @param tid The identifier of the thread trying to acquire the lock (0 or 1).
     */
    void lock(int tid);

    /**
     * @brief Unlock the critical section for a thread.
     *
     * @param tid The identifier of the thread releasing the lock (0 or 1).
     */
    void unlock(int tid);

private:
    /**
     * @brief Determine the identifier of the current thread.
     *
     * @param tid The identifier of the thread trying to acquire the lock (0 or 1).
     * @return The identifier of the current thread (0 or 1).
     */
    int my(int tid);

    /**
     * @brief Determine the identifier of the other thread.
     *
     * @param tid The identifier of the thread trying to acquire the lock (0 or 1).
     * @return The identifier of the other thread (0 or 1).
     */
    int other(int tid);

    bool mem_order;
};

#endif // MY_ATOMICS_H
