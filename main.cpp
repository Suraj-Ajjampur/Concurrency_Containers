/********************************************************************
 * @author Suraj Ajjampur
 * @file main.cpp
 * 
 * @brief Entry point of the concurrent containers program which calls the functions 
 * based on multiple test cases
 * 
 * @date 14 Dec 2023
********************************************************************/

#include "trieber_stack.h"
#include "msq.h"
#include "sgl.h"
#include "elimination.h"
#include <iostream>
#include <getopt.h>
#include <fstream>
#include "flat_combining.h"

using namespace std;

// Global variables
int NUM_THREADS = 5;
string data_structure = ""; 
string optimization = "";
string inputFile = "";


// Function to print my name
void printName() {
    cout << "Suraj Ajjampur" << endl;
}

// Function to sort integers in a file and print to another file
void DS_Wrapper(const string& inputFile, const string& data_structure, const string &optimization, int NUM_THREADS) {
    // Open the input file
    ifstream inFile(inputFile);
    
    // Check if the input file opened successfully
    if (!inFile.is_open()) {
        cerr << "Error: Could not open the input file." << endl;
        return;
    }

    // Read integers from the input file into a vector
    vector<int> numbers;
    int num;
    while (inFile >> num) {
        numbers.push_back(num);
    }

    // Close the input file
    inFile.close();
    
    // Start measuring time
    auto start_time = chrono::high_resolution_clock::now();

    // Call the appropriate test function based on the Container and Optimization Specified
    if (data_structure == "SGLQueue") {
         if (optimization == "none"){
            sgl_queue_test(numbers,NUM_THREADS);
         }else if(optimization == "Flat-combining"){
            sgl_queue_fc_test(numbers, NUM_THREADS);    // Call SGL Stack Test with Flat-Combining
        }else{cout << "Invalid optimization Selected " << endl; return;} 
    } else if (data_structure == "SGLStack") {
        if (optimization == "none"){
            sgl_stack_test(numbers, NUM_THREADS); // Call SGL Stack with no optimization
        }else if(optimization == "Elimination"){
            sgl_stack_elimination_test(numbers,NUM_THREADS);                    // Call SGL Stack Test with Elimination 
        }else if (optimization == "Flat-combining"){
            sgl_stack_fc_test(numbers, NUM_THREADS);
        }    
        else{cout << "Invalid optimization Selected " << endl; return;} 
    }
     else if (data_structure == "TS") {
        if (optimization == "none"){
            treiber_stack_test(numbers, NUM_THREADS); // Call the mergesort function
        }else if(optimization == "Elimination"){
            treiber_stack_elimination_test(numbers, NUM_THREADS);
        }else{cout << "Invalid optimization Selected " << endl; return;} 

    } else if (data_structure == "msqueue") {
        ms_queue_test(numbers,NUM_THREADS);
    } else {
        cerr << "Error: Invalid data_structure specified." << endl;
        return;
    }
    // Stop measuring time
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end_time - start_time);

    // Calculate and print elapsed time in nanoseconds
    cout << "\033[1mTime taken: \033[32m" << duration.count() << " microseconds\033[0m" << endl;
}

/**
 * Prints the usage instructions for the program with ANSI escape codes for formatting.
 * This function utilizes terminal colors and text styles to enhance the readability of the instructions.
 */
void Execution_instructions() {
    // ANSI escape codes for formatting
    const char* bold_on = "\033[1m";
    const char* underline_on = "\033[4m";
    const char* color_green = "\033[32m";
    const char* color_yellow = "\033[33m";
    const char* reset_format = "\033[0m";

    cout << bold_on << "Usage instructions:" << reset_format << endl;
    cout << "  " << underline_on << "--name" << reset_format << "\t\tDisplay the author's name." << endl;
    cout << "  " << underline_on << "--help" << reset_format << "\t\tShow this help message." << endl;
    cout << "  " << underline_on << "-i, --input" << reset_format << "\t\tSpecify the source input file containing data to process." << endl;
    cout << "  " << underline_on << "-t, --threads" << reset_format << "\t\tSet the number of threads for execution (must be a positive integer)." << endl;
    cout << "  " << underline_on << "--data_structure" << reset_format << "\tChoose the data structure to use. Options: " << color_yellow << "SGLQueue, SGLStack, TS (Treiber Stack), msqueue" << reset_format << "." << endl;
    cout << "  " << underline_on << "--optimization" << reset_format << "\tSelect the optimization technique. Options: " << color_yellow << "none, Elimination, Flat-combining" << reset_format << "." << endl;
    cout << "\n" << bold_on << "Example:" << reset_format << endl;
    cout << color_green << "  ./containers --input sourcefile.txt --threads 4 --data_structure=TS --optimization=Elimination" << reset_format << endl;
    cout << "This command will process 'sourcefile.txt' using the Treiber Stack with the Elimination optimization across 4 threads." << endl;
}


/**
 * Main function to process command-line arguments and execute data_structure on input data.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return An integer indicating the exit status.
 */
int main(int argc, char* argv[]) {
    // Check if any command-line arguments are provided
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " [--name] [--help] [-i sourcefile.txt] [-t NUMTHREADS] [--data_structure=<SGLQueue,SGLStack,TS,msqueue>] [--optimization=<none,Elimination,Flat-combining,>]" << endl;
        return 1;
    }

    // variable to parse command line arguements
    int c;

    //Define getopt long struct with commands
    static struct option long_options[] = {
        {"name", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {"threads", required_argument, 0, 't'},
        {"input", required_argument, 0, 'i'},
        {"data_structure", required_argument, 0, 'd'},
        {"optimization", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };
    
    while (1) {
        int option_index = 0;

        // Parse the command line options
        c = getopt_long(argc, argv, "nt:d:i:o:", long_options, &option_index);
        
        // Detect the end of the options
        if (c == -1)
            break;
        switch (c) {
            case 'n':
                // Print name
                cout << "Suraj Ajjampur" << endl;
                return 0;
            
            case 'h':
                // Executables generated should provide execution instructions when given a -h flag.
                Execution_instructions();
                return 0;

            case 't':
                // Set the number of threads
                NUM_THREADS = stoi(optarg);
                break;

            case 'd':
                // Set the lock type
                data_structure = optarg;
                break;

            case 'o':
                // Set the input file
                optimization = optarg;
                break;

            case 'i':
                // Set the input file
                inputFile = optarg;
                break;

            case '?':
                // Handle invalid options
                cerr << "Error: Invalid option." << endl;
                return 1;

            default:
                abort();
        }
    }     

    // Check if input and output files are provided
    if (inputFile.empty()) {
        cerr << "Error: Input file is empty" << endl;
        return 1;
    }

    DEBUG_MSG("Data-Structure Selected is " << data_structure);
    DEBUG_MSG("Inputfile Selected is " << inputFile);
    DEBUG_MSG("Optimization Selected is " << optimization);
    DEBUG_MSG("Numthreads Selected is " << NUM_THREADS);

    // Sort and print the input file to the output file
    DS_Wrapper(inputFile, data_structure, optimization, NUM_THREADS);

    return 0;
}