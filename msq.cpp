/*****************************************************************
 * @author Suraj Ajjampur
 * @file msq.cpp
 * 
 * @brief This C++ source file implements the Micheal & Scott Queue, which is a non-blocking linearizable queue 
 *        which enqueues from the tail and dequeues from the head
 * 
 * @date 5 Dec 2023
********************************************************************/
#include <my_atomics.h>

class msqueue{
    public:
        class node{
            public:
                node(int v):val(v){}
                int val; atomic<node*> next;
                };
        atomic<node*> head, tail;
        msqueue();
        void enqueue(int val);
        int dequeue();
        };
        msqueue::msqueue(){
        node* dummy = new node(DUMMY);
        head.store(dummy);
        tail.store(dummy);
};

/** Make sure tail is up-to-date
 * Add new node (linearization point)
 * Update the 
 * 
 * @param val to be enqueued
 */ 
void msqueue::enqueue(int val){
    node *t, *e, *n;
    n = new node(val);
    while(true){
    t = tail.load(); e = t->next.load();
    if(t == tail.load()){
    if(e==NULL && cas(t->next,NULL,n,ACQ_REL)){break;}
    else if(e!=NULL){cas(tail,t,e,ACQ_REL);} 
    } 
    }
    cas(tail,t,n,ACQ_REL);
}

/** 
 */
int msqueue::dequeue(){
    node *t, *h, *n; 
    while(true){
        h=head.load(); t=tail.load(); n=h->next.load();
        if(h==head.load()){
            if(h==t){
                if(n==NULL){return NULL;}
            else{cas(tail,t,n,ACQ_REL);}
            }
        else{
        int ret=n->val;
        if(cas(head,h,n,ACQ_REL)){return ret;}
        }
        }
 }
}
