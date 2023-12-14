/*****************************************************************
 * @author Suraj Ajjampur
 * @file main.cpp
 * 
 * @brief Entry point of the concurrent containers program which calls the functions 
 * based on multiple test cases
 * @date 14 Dec 2023
********************************************************************/

#include "trieber_stack.h"
#include "msq.h"
#include "sgl.h"
#include <iostream>

int main(void){
    push3_pop_till_empty();
    push_pop();

    testBasicQueueOperations();
    testConcurrentuQeueOperations();
    
    testConcurrentPushPop();
    
    testBasicSGLQueueOperations();
    testConcurrentSGLQueueOperations();
    /*********** Testing for MSQueue here **********************/
    
    /************ Elimination here *****************************/
    return 0;
}

