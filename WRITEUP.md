## Experimental results as required by the prompt

In this section, I present the outcomes of executing my test.sh script, which was designed to assess the performance of various data structures under different conditions. This script served as a comprehensive test suite for evaluating the efficiency and scalability of four specific data structures: SGLQueue, SGLStack, TS, and msqueue. These structures were tested across a range of thread counts, from 1 to 100, incrementing in steps of 5. This approach allowed for a detailed analysis of how each data structure performed under varying levels of concurrency.

The script began with the execution of the make clean command to ensure a clean build environment, followed by clear and make commands to compile the latest version of the program. This ensured that the tests were run on the most recent codebase. The use of the --name and --help flags with the ./containers command provided basic information about the program and its usage, respectively.

The core of the script lay in its nested loop structure, where it iterated over each data structure with each specified thread count. This thorough testing framework was crucial for evaluating the performance characteristics of each data structure under different levels of parallelism. For each combination of data structure and thread count, I executed the ./containers program with the input_test_files/256in1-10000.txt input file, specifying the data structure and the number of threads to be used.

## Analysis of results using perf as necessary to support explanations

## A description of your code organization

## A description of every file submitted

## Compilation instructions

## Execution instructions, particulary for any results presented in the write-up

## Any extant bugs