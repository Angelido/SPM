#!/bin/bash

# Output file for the benchmark results
OUTPUT_FILE="mergesort_par_results.txt"
EXECUTABLE="./mergesort_par"

# Define the parameters for the benchmark
SIZES=(1K 10K 100K 1M 10M 100M)     
PAYLOADS=(8 16 32 64 128 256)
THREADS=(1 2 4 8 16 32)
REPEATS=3

echo "Starting benchmark for mergesort_par_new" > "$OUTPUT_FILE"
echo "Date: $(date)" >> "$OUTPUT_FILE"
echo "------------------------------------------" >> "$OUTPUT_FILE"

# Start the benchmark loop
for N in "${SIZES[@]}"; do
  for P in "${PAYLOADS[@]}"; do
    for T in "${THREADS[@]}"; do
      for RUN in $(seq 1 $REPEATS); do
        echo "Running N=$N, P=$P, T=$T (Run $RUN)" | tee -a "$OUTPUT_FILE"
        echo "CMD: $EXECUTABLE -s $N -r $P -t $T" >> "$OUTPUT_FILE"
        $EXECUTABLE -s $N -r $P -t $T >> "$OUTPUT_FILE" 2>&1
        echo "------------------------------------------" >> "$OUTPUT_FILE"
      done
    done
  done
done

echo "Benchmark completed at $(date)" >> "$OUTPUT_FILE"
