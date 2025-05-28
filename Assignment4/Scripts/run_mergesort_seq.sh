#!/bin/bash

# Output file for the benchmark results
OUTPUT_FILE="result_seq.txt"
> "$OUTPUT_FILE"  # Svuota il file se esiste già

# Define the parameters for the benchmark
sizes=(10K 100K 1M 10M 100M)
ranges=(8 16 32 64 128)

# Compute the benchmark loop
for size in "${sizes[@]}"; do
  for range in "${ranges[@]}"; do
    for run in {1..3}; do
      echo "Esecuzione $run - Size: $size, Range: $range" >> "$OUTPUT_FILE"
      ./mergesort_seq -s "$size" -r "$range" >> "$OUTPUT_FILE" 2>&1
      echo "" >> "$OUTPUT_FILE"
    done
  done
done

echo "Tutte le esecuzioni completate. Risultati salvati in $OUTPUT_FILE."
