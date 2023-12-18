## Experimental results as required by the prompt

In this section, I present the outcomes of executing my test.sh script, which was designed to assess the performance of various data structures under different conditions. This script served as a comprehensive test suite for evaluating the efficiency and scalability of four specific data structures: SGLQueue, SGLStack, TS, and msqueue. These structures were tested across a range of thread counts, from 1 to 100, incrementing in steps of 5. This approach allowed for a detailed analysis of how each data structure performed under varying levels of concurrency.

The script began with the execution of the make clean command to ensure a clean build environment, followed by clear and make commands to compile the latest version of the program. This ensured that the tests were run on the most recent codebase. The use of the --name and --help flags with the ./containers command provided basic information about the program and its usage, respectively.

The core of the script lay in its nested loop structure, where it iterated over each data structure with each specified thread count. This thorough testing framework was crucial for evaluating the performance characteristics of each data structure under different levels of parallelism. For each combination of data structure and thread count, I executed the ./containers program with the input_test_files/256in1-10000.txt input file, specifying the data structure and the number of threads to be used.

## Analysis of results using perf as necessary to support explanations

I measured the performance of the containers program using the perf tool.

## Performance Table Stacks

| Stack Container Optimization    | Execution time (us) | L1 cache miss (%) | 
|--------------|-------------|------------------|
| SGL - no opimization              | 4556   | 11.83% | 
| SGL - Elimination           | 3887  | 12.61% | 
| SGL - Flat-combining           | 6606  | 5.54% | 
| Treiber-Stack - no opimization          | 23150  | 16.82% | 
| Treiber-Stack - Elimination           | 21112  | 17.85% | 

<img width="763" alt="image" src="https://github.com/ecen4313-fl23/finalproject-Suraj-Ajjampur/assets/134313353/37cd542b-3b68-47ae-bed6-af237b7a715e">

## Performance Table Queue

| Queue Container    | Runtime (us) | L1 cache miss (%) |
|--------------|-------------|------------------|
| SGL              | 4705   | 12.13% | 
| M&S Queue              | 4262   | 11.96% |


<img width="770" alt="image" src="https://github.com/ecen4313-fl23/finalproject-Suraj-Ajjampur/assets/134313353/20aa5f3e-8956-4356-81d0-29590af43e79">

## Performance Summary

**Single Global Lock (SGL) Stack:** The SGL Stack without optimization has the lowest execution time at 4556 microseconds, but with a moderate L1 cache miss rate of 11.83%. The Elimination optimization slightly increases the execution time to 3887 microseconds while also increasing the cache miss rate to 12.61%. The Flat-combining optimization, while reducing the L1 cache miss rate to 5.54%, significantly increases the execution time to 6606 microseconds. This suggests that while Flat-combining is effective in reducing cache misses, it may introduce overheads leading to longer execution times.

**Treiber Stack:** The Treiber Stack without optimization has a considerably higher execution time of 23150 microseconds with an L1 cache miss rate of 16.82%. Applying the Elimination optimization reduces both the execution time (to 21112 microseconds) and the cache miss rate (to 17.85%). The reduction in execution time with Elimination indicates its effectiveness in enhancing performance, despite a slight increase in cache misses.

For queues:

**SGL Queue:** The SGL Queue's performance is marked by an execution time of 4705 microseconds and an L1 cache miss rate of 12.13%, indicating a balanced performance in terms of speed and cache efficiency.

**Michael & Scott (M&S) Queue: **This queue shows a slightly better performance with an execution time of 4262 microseconds and a marginally lower cache miss rate of 11.96%, suggesting that the M&S Queue might be slightly more efficient in terms of both execution speed and cache utilization compared to the SGL Queue.

These results highlight the trade-offs between different data structures and optimizations, especially regarding execution speed and cache efficiency. While some optimizations can decrease cache misses, they might also increase the execution time, and vice versa.


## A description of your code organization

My code is organized in terms of each data-structure has it's own class. For exam the Trieber stack has the class tstack where my node structure is defined as well as my push and pop member functions. This class is the header fie whereas my functions are defined in the c files. Each of these data-structure files has a test functions which takes in a vector on values to perform operations on. For example, `treiber_stack_test` exist for that reason. Where threads are being spawned to push and pop from the Treiber stack, using wrapper functions. The popCount is an atomic integer which is incremented atomically at each pop. Finally we check if the popCount is equal to the number of values in the vector to verify that the Stack works.

```
void Push(tstack& stack, int val) {
    stack.push(val);
}

void Pop(tstack& stack, std::atomic<int>& popCount) {
    int val = stack.pop();
    if ( val != -1) {
        popCount.fetch_add(1, RELAXED);
    }else{DEBUG_MSG("Stack is empty");}
}
```

This is how stacks are being verified.


For queues, similarly we have the classes written in the header files for example the `msqueue` class is in the msq.h file with the node and the enqueue and dequeue functions and either prototype. The members are written in the msq.cpp files, which includes a `ms_queue_test` function which given a set of values and a specified number of threads, test the M&S lock-free queue. The number of threads are divided queually between enqueue and dequeue operations. It validates the test by comparing the dequeued values against the expercted sum calculated from the input values. The enqueue and dequeue are wrapper functions which call the msqueue instance operations

```
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
```

## A description of every file submitted
- `main.cpp`: The code's entry point is the main function, which reads arguments given while program execution using get-opt long, including the number of threads. It calls the 'DS_Wrapper' function which executes a test for the specified data structure with a given optimization strategy using input from a file.
- `my_atomics.cpp`: This is where all the concurrency primitives are defined. They are declared as functions or classes using atomic variables. This is the main foundation of the program.
- `Makefile`: This makefile defines compilation rules and dependencies for two C++ executables: mysort and counter. It specifies the compiler (g++) and compiler flags (-pthread -O0 -std=c++2a -mcx16). It lists the source files for each executable and their corresponding object files. The default target is to build the container executable. Rules for building the executable and compiling source files are defined, and there's also a clean rule to remove object files and executables.
- `trieber_stack` - implements Trieber stack which is a non-blocking data structure. It is linearizable and lock-free, The Race against reclaimation is solved but not the ABA problem as I am unclear about smart pointer implementation.
- `sgl` - This source file implements a Stack and Queue to be used in a multithreaded application using a Single-Global Lock.
- `msq.cpp` -  implements the Micheal & Scott Queue, which is a non-blocking linearizable queue which enqueues from the tail and dequeues from the head
- `input_test_files` - There are the files I have tested my containers againts where each of these files are being read and the values are being read into a vector which is being processed.
- `flat_combining.cpp` - implements concurrent containers which uses a single global lock optimized using the Flat combining Method.
- `elimination.cpp` - implements stacks using the elimination method in order to deal with contention issues.
- `test.sh` - this provides a method to clean, build and run the program for different data structures and optimization with different number of threads. I wrote this to stress test my program and identify some corner cases.
  
## Compilation instructions
```
./test.sh 
```
is quite comprehensive tests that runs and does a clean and make it also automatically tests for all Containers types and their respective optimizations too.

## Execution instructions, particulary for any results presented in the write-up

I measured the performance of the containers program using the perf tool.

Steps I took for this, 

Copied my entire directory to the linux machine using scp command

```
scp -r ./finalproject-Suraj-Ajjampur suaj6464@ecen4313-fl23-1.int.colorado.edu:~
```

Logged into the machine using the SSH command

```
ssh suaj6464@ecen4313-fl23-1.int.colorado.edu
```

Gave the below arguements for the perf command and used it to obtain the cache miss rate and the execution time for each of my containers and optimization using 50 threads while entering and removing 256 values.

```
perf stat -e L1-dcache-load-misses -e ./container -i input_test_files/256in1-10000.txt -t 50 --data_structure=TS --optimization=Elimination
```

## Any extant bugs

### Implementation of the Flat Combining SGL Queue has a bug which I am unable to resolve. 

Description: 

During the execution of the SGLQueue with Flat-Combining optimization, a discrepancy in the sum of dequeued values was observed compared to the expected sum. This issue manifests when multiple threads are concurrently enqueuing and dequeuing values from the queue. The debug logs indicate frequent occurrences of "Lock acquisition failed in enqueue, using combining array" messages, suggesting a high contention rate and possible inefficiencies in the lock acquisition strategy. Additionally, the combining operations, intended to process multiple queue operations in a single lock acquisition, seem to be executing successfully, but the overall consistency of the queue operations is compromised.

Possible Causes:
**High Contention on Lock:** The logs show repeated attempts at lock acquisition failing during enqueues, leading to a reliance on the combining array. This pattern suggests that the flat-combining mechanism might be overwhelmed or improperly managed under high concurrency.

**Improper Management of Combining Array:** It is possible that the combining array is not correctly processing all queued operations. The logic in the combine() function may need a review to ensure that operations are correctly identified, executed, and marked as completed.

**Incorrect Sum Calculation:** The final validation step, which compares the sum of dequeued values with the expected sum, may be encountering anomalies due to the above issues. If dequeues are not correctly synchronized with enqueues, or if some enqueue operations are lost or duplicated in the process, this would lead to an inaccurate sum.


### I haven't yet solved the ABA problem using smart pointers, hence for extremely big vector inputs the memory of the system isn't enough. Garbage Collection issue still persists.
