#!/bin/bash

# Script for running tests on minizseq and minizpar

# General settings
date > run_tests.log
REPETITIONS=10
OUTPUT_FILE="result.csv"
THREAD_LIST=(1 2 4 8 16 32)

# Commands to run
declare -a COMMANDS=(
    "-r 0 -C 0 -q 1 big_files"
    "-r 0 -D 1 -q 1 big_files"
    "-r 0 -C 0 -q 1 small_files"
    "-r 0 -D 1 -q 1 small_files"
    "-r 1 -C 0 -q 1 nested_files"
    "-r 1 -D 1 -q 1 nested_files"
)

# CSV header
echo "Program,Threads,CommandIndex,Command,Run,Output" > "$OUTPUT_FILE"

# ==== minizseq section ====
echo "=== Running minizseq ===" | tee -a run_tests.log
for ((run=1; run<=REPETITIONS; run++)); do
    echo "minizseq: round $run of $REPETITIONS" | tee -a run_tests.log
    for i in "${!COMMANDS[@]}"; do
        cmd="${COMMANDS[$i]}"
        echo "  ▶ [#$(($i+1))] ./minizseq $cmd" | tee -a run_tests.log
        output=$(./minizseq $cmd 2>&1)
        printf "minizseq,1,%d,%q,%d,%q\n" "$((i+1))" "$cmd" "$run" "$output" >> "$OUTPUT_FILE"
    done
done

# ==== minizpar section ====
echo "=== Running minizpar ===" | tee -a run_tests.log
for threads in "${THREAD_LIST[@]}"; do
    for ((run=1; run<=REPETITIONS; run++)); do
        echo "minizpar: threads=$threads, round $run of $REPETITIONS" | tee -a run_tests.log
        for i in "${!COMMANDS[@]}"; do
            cmd="${COMMANDS[$i]}"
            
            # Set the number of threads for OpenMP
            export OMP_NUM_THREADS=$threads
            
            echo "  ▶ [#$(($i+1))] ./minizpar $cmd" | tee -a run_tests.log
            output=$(./minizpar $cmd 2>&1)

            # Print the output to the console
            printf "minizpar,%d,%d,%q,%d,%q\n" "$threads" "$((i+1))" "$cmd" "$run" "$output" >> "$OUTPUT_FILE"
        done
    done
done

# End of script

echo "All tests completed at $(date)" | tee -a run_tests.log

echo "Results saved to $OUTPUT_FILE" | tee -a run_tests.log
