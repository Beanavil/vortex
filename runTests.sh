#!/bin/bash

# Define the applications, matrix sizes, and cache configurations
#apps=("tmul_submatrix" "tmul_base_tiles")
apps=("tmul_base")

sizes=(4 8 16 32 64)
cache_options=("" "--l2cache" "--l2cache --l3cache")  # Include combinations of cache configurations

# Output file
output_file="test_results.txt"

# Clear or create the output file
echo "" > "$output_file"

# Loop over each application
for app in "${apps[@]}"; do
    # Loop over each matrix size
    for size in "${sizes[@]}"; do
        # Loop over each cache configuration
        for cache in "${cache_options[@]}"; do
            # Execute the test 5 times for the current size, app, and cache configuration
            for run in {1..5}; do
                echo "Running $app with size $size, cache setting '$cache', iteration $run"

                # Construct the command line
                cmd="./ci/blackbox.sh --driver=rtlsim --app=$app --args=\"-n$size\" --perf=1 $cache"
                echo "Executing command: $cmd"
                
                # Execute the command and capture the output
                output=$(eval $cmd)
                echo $output >> aux.txt  # Debug output, can be removed if no longer needed

                # Extract the relevant information
                size_matrix=$(echo "$output" | grep -o "Size matrix: [0-9x]*" | awk '{print $3}')
                elapsed_time=$(echo "$output" | grep -o "Elapsed time: [0-9]* ms" | awk '{print $3 " " $4}')
                instrs=$(echo "$output" | grep "PERF: instrs=" | awk -F'[,=]' '{print $2}')
                cycles=$(echo "$output" | grep "PERF: cycles=" | awk -F'[,=]' '{print $2}')
                ipc=$(echo "$output" | grep "IPC=" | awk -F'[,=]' '{print $2}')
                ibuffer_stalls=$(echo "$output" | grep "PERF: ibuffer stalls=" | awk -F'[=]' '{print $2}')
                scheduler_stalls=$(echo "$output" | grep "PERF: scheduler stalls=" | awk -F'[=]' '{print $2}')

                # Extracting the entire issue stalls line correctly
                issue_stalls=$(echo "$output" | grep "PERF: issue stalls=")

                # Extract details from "PERF: issue stalls"
                issue_stalls_total=$(echo "$issue_stalls" | awk -F'[=(]' '{print $2}')
                issue_stalls_alu=$(echo "$issue_stalls" | awk -F'alu=' '{print $2}' | awk -F'%' '{print $1}')
                issue_stalls_fpu=$(echo "$issue_stalls" | awk -F'fpu=' '{print $2}' | awk -F'%' '{print $1}')
                issue_stalls_lsu=$(echo "$issue_stalls" | awk -F'lsu=' '{print $2}' | awk -F'%' '{print $1}')
                issue_stalls_sfu=$(echo "$issue_stalls" | awk -F'sfu=' '{print $2}' | awk -F'%' '{print $1}')

                # Append the results to the output file
                echo "App: $app, Size: $size_matrix, Cache: $cache, Elapsed Time: $elapsed_time, Instrs: $instrs, Cycles: $cycles, IPC: $ipc, ibuffer_stalls: $ibuffer_stalls, scheduler_stalls: $scheduler_stalls, Issue Stalls Total: $issue_stalls_total, ALU: $issue_stalls_alu%, FPU: $issue_stalls_fpu%, LSU: $issue_stalls_lsu%, SFU: $issue_stalls_sfu%" >> "$output_file"
            done
        done
    done
done

echo "Tests completed, results saved in $output_file"

