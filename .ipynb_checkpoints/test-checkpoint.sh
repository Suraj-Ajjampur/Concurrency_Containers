#!/bin/bash

make clean && clear && make

./containers --name
./containers --help

input_file="input_test_files/256in1-10000.txt"
data_structures=("SGLQueue" "SGLStack" "TS" "msqueue")
thread_counts=(1 5 10 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100)

# Loop through the specified thread counts
for num_threads in "${thread_counts[@]}"; do
    echo "=========================="
    echo "Testing with $num_threads threads"
    echo "=========================="

    # Loop through data structures
    for data_structure in "${data_structures[@]}"; do
        echo "Testing $data_structure with no optimization:"
        ./containers -i $input_file --data_structure=$data_structure --optimization=none -t $num_threads

        # Apply Elimination only for stacks
        if [ "$data_structure" == "SGLStack" ] || [ "$data_structure" == "TS" ]; then
            echo "Testing $data_structure with Elimination optimization:"
            ./containers -i $input_file --data_structure=$data_structure --optimization=Elimination -t $num_threads
        fi

        # Apply Flat Combining only for SGLStack 
        if [ "$data_structure" == "SGLStack" ]; then
            echo "Testing $data_structure with Flat-combining optimization:"
            ./containers -i $input_file --data_structure=$data_structure --optimization=Flat-combining -t $num_threads
        fi
    done
done
