#!/bin/bash

# Define the applications, matrix sizes, and cache configurations
apps=("tmul_submatrix" "tmul_base")
sizes=(4 8 16 32 64)

# Output file
output_file="test_results_cache.txt"

# Clear or create the output file
echo "" > "$output_file"

# Loop over each application
for app in "${apps[@]}"; do
    # Loop over each matrix size
    for size in "${sizes[@]}"; do
        echo "Running $app with size $size, cache setting '$cache', iteration $run"

        # Construct the command line
        cmd="./ci/blackbox.sh --driver=rtlsim --app=$app --args=\"-n$size\" --perf=2"
        echo "Executing command: $cmd"

        # Execute the command and capture the output
        output=$(eval $cmd)
        echo "$output" >> aux.txt  # Debug output, can be removed if no longer needed

        # Extract the relevant information
        size_matrix=$(echo "$output" | grep -o "Size matrix: [0-9x]*" | awk '{print $3}')
        elapsed_time=$(echo "$output" | grep -o "Elapsed time: [0-9]* ms" | awk '{print $3 " " $4}')
        smem_reads=$(echo "$output" | grep "PERF: core0: smem reads=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        smem_writes=$(echo "$output" | grep "PERF: core0: smem writes=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        smem_bank_stalls=$(echo "$output" | grep "PERF: core0: smem bank stalls=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        icache_reads=$(echo "$output" | grep "PERF: core0: icache reads=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        icache_read_misses=$(echo "$output" | grep "PERF: core0: icache read misses=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        icache_mshr_stalls=$(echo "$output" | grep "PERF: core0: icache mshr stalls=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        dcache_reads=$(echo "$output" | grep "PERF: core0: dcache reads=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        dcache_writes=$(echo "$output" | grep "PERF: core0: dcache writes=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        dcache_read_misses=$(echo "$output" | grep "PERF: core0: dcache read misses=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        dcache_write_misses=$(echo "$output" | grep "PERF: core0: dcache write misses=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        dcache_bank_stalls=$(echo "$output" | grep "PERF: core0: dcache bank stalls=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        dcache_mshr_stalls=$(echo "$output" | grep "PERF: core0: dcache mshr stalls=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        memory_requests=$(echo "$output" | grep "PERF: memory requests=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        memory_latency=$(echo "$output" | grep "PERF: memory latency=" | awk -F'[= ]' '{print $3}')
        instrs=$(echo "$output" | grep "PERF: instrs=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        cycles=$(echo "$output" | grep "PERF: cycles=" | awk -F'[=]' '{print $2}' | awk '{print $1}')
        ipc=$(echo "$output" | grep "IPC=" | awk -F'[=]' '{print $2}' | awk '{print $1}')


        # Append the results to the output file
        echo "App: $app, Size: $size_matrix, Cache: $cache, Elapsed Time: $elapsed_time, smem_reads: $smem_reads, smem_writes: $smem_writes, smem_bank_stalls: $smem_bank_stalls, icache_reads: $icache_reads, icache_read_misses: $icache_read_misses, icache_mshr_stalls: $icache_mshr_stalls, dcache_reads: $dcache_reads, dcache_writes: $dcache_writes, dcache_read_misses: $dcache_read_misses, dcache_write_misses: $dcache_write_misses, dcache_bank_stalls: $dcache_bank_stalls, dcache_mshr_stalls: $dcache_mshr_stalls, memory_requests: $memory_requests, memory_latency: $memory_latency, Instrs: $instrs, Cycles: $cycles, IPC: $ipc" >> "$output_file"
    done
done

echo "Tests completed, results saved in $output_file"
