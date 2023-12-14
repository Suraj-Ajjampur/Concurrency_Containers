/*****************************************************************
 * @author Suraj Ajjampur
 * @file my_atomics.cpp
 * 
 * @brief This C++ source file implements atomic operations and memory orderings
 *        using C++ atomics for test-and-set, fetch-and-increment, compare-and-swap,
 *        and a variant of compare-and-swap with a return value.
 * @date 24 Oct 2023
********************************************************************/

#include "my_atomics.h"

/** Atomically check if value is false, if it is, return true
 * or else return false
 * 
 * 
 */
bool tas(atomic<bool>& x, std::memory_order MEM){
 bool expected = false;
 bool desired = true;
 return x.compare_exchange_strong(expected, desired, MEM); 
}
// The below locks are all fifo based 

/** If 5 threads call this function then first one to arrive will set lock x to true,
 *  All other flags will read true and the tas will return false. Hence the lock won't work for them
 */ 
void tas_lock(atomic<bool>& x){
    while(tas(x,SEQ_CST) == false){}
}

/** Store false to atomic bool x
 */ 
void tas_unlock(atomic<bool>& x){
    return x.store(false, SEQ_CST);
}

/** Designed to avoid any cache coherence traffic by doing only a load to avoid writing (tas) each time 
 *  which causes bus contention.
 */ 
void ttas_lock(atomic<bool>& x){
    while(x.load(RELAXED) == true ||
    tas(x,SEQ_CST) == false){}
}

void ttas_unlock(atomic<bool>& x){
    return x.store(false, SEQ_CST);
}

int fai(atomic<int>& x, int amount, std::memory_order MEM){
 return x.fetch_add(amount, MEM); 
}

void ticket_lock(atomic<int>& next_num, atomic<int>& now_serving){
    int my_num = fai(next_num, 1 , SEQ_CST);
    while(now_serving.load(SEQ_CST) != my_num){}
}

void ticket_unlock(atomic<int>& now_serving){
    fai(now_serving, 1 ,SEQ_CST);
}

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

/** Atomically check if the old value is what you expect, if it is, replace it
 * with a new value and returns value found.
 * 
 * @return Returns the expected value found on success
 */ 
template <typename T>
T vcas(atomic<T> &x, T expected, T desired, std::memory_order MEM){
 T expected_ref = expected;
 x.compare_exchange_strong(expected_ref, desired, MEM);
 return expected_ref;
}

//Parameterized constructor for MCSLock
MCSLock::MCSLock(atomic<Node*>& tail) : tail(nullptr) {} // Initialization

void MCSLock::acquire(Node* myNode){
    Node* oldTail = tail.load(SEQ_CST); //Read current value of tail
    myNode->next.store(nullptr, RELAXED); // Set to NULL as we don't know what it is. (Initialization on my node)
    while(!cas(tail,oldTail,myNode,SEQ_CST)){
        oldTail = tail.load(SEQ_CST);
    }
    // if oldTail == NULL, we’ve 
    // acquired the lock
    // otherwise, wait for it
    if(oldTail != nullptr) { //This mean we are not the head of the list hence we have to wait
    myNode->wait.store(true,RELAXED);
    oldTail->next.store(myNode,SEQ_CST); // Updating the old tail value
    while (myNode->wait.load(SEQ_CST)) {} //Spin locally till it becomes false
    }
}

void MCSLock::release(Node* myNode){
    Node* m = myNode; //Appending my node to the queue
    if(cas<MCSLock::Node*>(tail,m, nullptr, SEQ_CST)) { //Swinging tail from the old value to the new value
    // no one is waiting, and we just 
    // freed the lock
    } 
    else{// hand lock to next waiting thread
    while(myNode->next.load(SEQ_CST)==NULL){} //Spin until notified
    myNode->next.load(SEQ_CST)->wait.store(false,SEQ_CST);
    }
}

SenseBarrier::SenseBarrier(int numThreads) : cnt(0), sense(0), N(numThreads) {}

void SenseBarrier::ArriveAndWait() {
        thread_local bool my_sense = 0; // Initialize to false

        // Flip the sense barrier every iteration
        if (my_sense == 1) {
            my_sense = 1;
        } else {
            my_sense = 0;
        }

        int cnt_cpy = fai(cnt, 1, SEQ_CST); //Increments the cnt by 1 and assigns to local copy

        // Last thread to arrive resets the counter and sense
        if (cnt_cpy == N - 1) {
            cnt.store(0, RELAXED);
            sense.store(my_sense, RELAXED);
        } else { // Not the last thread
            // Wait for other threads to synchronize on the same sense
            while (sense.load(SEQ_CST) != my_sense) {}
        }
}

void SenseBarrier::ArriveAndWaitRel(){
    
}

int Petersons::my(int tid) {
    // Define the logic to determine the current thread
    return tid; // For simplicity, assuming tid is already the correct identifier
}

int Petersons::other(int tid) {
    // Define the logic to determine the other thread
    return 1- tid; // Assuming there are only two threads with identifiers 0 and 1
}

Petersons::Petersons(bool memory_order) {
    DEBUG_MSG("Start Construction!!"); 
    if(memory_order == SEQ_CONISTENCY){
        //Reset the desire of both the threads to acquire the lock
        desires[0].store(false, SEQ_CST);
        desires[1].store(false, SEQ_CST);
        //Give the turn to one of them
        turn.store(0, SEQ_CST);
    }else{ // For RELEASE_CONSISTENCY
        //Reset the desire of both the threads to acquire the lock
        desires[0].store(false, RELEASE);
        desires[1].store(false, RELEASE);
        //Give the turn to on of them
        turn.store(0, RELEASE);
    }
    mem_order = memory_order;
    DEBUG_MSG("Peterson's constructed!!");
}

void Petersons::lock(int tid){ 
    DEBUG_MSG("Peterson's lock start");
    if(mem_order == SEQ_CONISTENCY){
        // Say you want to acquire lock 
        desires[my(tid)].store(true,SEQ_CST); 
        
        // But, first give the other thread 
        // the chance to acquire lock 
        // Essentially yeild the lock to the other thread
        turn.store(other(tid),SEQ_CST); 
        
        // Wait until the other thread 
        // looses the desire 
        // to acquire lock or it is your 
        // turn to get the lock. 
        /* We can execute the critical section if either the other thread doesn't desire the lock
        or the other thread does desire the lock and it's our turn*/
        while(desires[other(tid)].load(SEQ_CST)
        && turn.load(SEQ_CST) == other(tid)){
            //Spin till it's safe to acquire the lock
        }
    }else{ // Release Consistency
        // Say you want to acquire lock 
        desires[my(tid)].store(true,RELEASE); 
        // But, first give the other thread 
        // the chance to acquire lock 
        // Essentially yeild the lock to the other thread
        turn.store(other(tid),RELEASE); 
        atomic_thread_fence(SEQ_CST);
        // Wait until the other thread 
        // looses the desire 
        // to acquire lock or it is your 
        // turn to get the lock. 
        /* We can execute the critical section if either the other thread doesn't desire the lock
        or the other thread does desire the lock and it's our turn*/
        while(desires[other(tid)].load(ACQUIRE)
        && turn.load(ACQUIRE) == other(tid)){
            //Spin till it's safe to acquire the lock
        }
    }
    //cout << "Peterson's lock acquired!!" << endl; 
}

void Petersons::unlock(int tid) { 
    // You do not desire to acquire lock
    // allowing the other thread to acquire the lock. 
    if(mem_order == SEQ_CONISTENCY){
        desires[my(tid)].store(false,SEQ_CST); 
    }else{
        desires[my(tid)].store(false,RELEASE); 
        atomic_thread_fence(SEQ_CST);
    }
    DEBUG_MSG("Peterson's lock released!!");
} 





