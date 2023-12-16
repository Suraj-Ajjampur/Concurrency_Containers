#!/bin/sh

make clean && clear && make

./containers --name

# Navigate to the input_test_files directory
cd input_test_files

# Loop through each test file and execute the Treiber_stack test
for file in *
do
  echo "Testing with file: $file"
  ../containers -i "$file" --data_structure=TS
done

# Navigate back to the original directory
cd ..
