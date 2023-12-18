#!/bin/bash

make clean && clear && make

./containers --name
./containers --help

input_file="input_test_files/256in1-10000.txt"
num_threads=4
data_structures=("SGLQueue" "SGLStack" "TS" "msqueue")

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
