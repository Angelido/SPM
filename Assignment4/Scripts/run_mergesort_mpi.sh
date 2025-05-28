#!/bin/bash

# Output file for the benchmark results
OUTPUT_FILE="result_distributed.txt"
PROGRAM="./mergesort_mpi"

echo "Risultati esecuzioni MergeSort distribuito" > "$OUTPUT_FILE"
echo "------------------------------------------" >> "$OUTPUT_FILE"

# Define the parameters for the benchmark
NODES_LIST=(1 2 4 8)
SIZES_LIST=(1M 2M 4M 8M 10M 100M)     # 1M, 10M, 100M
PAYLOADS_LIST=(8 16 32 64 128 256)
THREADS_LIST=(8 16 32)

# Compute the benchmark loop
for NODES in "${NODES_LIST[@]}"; do
  for SIZE in "${SIZES_LIST[@]}"; do
    for PAYLOAD in "${PAYLOADS_LIST[@]}"; do
      for THREADS in "${THREADS_LIST[@]}"; do
        echo "Running: Nodes=$NODES, Size=$SIZE, Payload=$PAYLOAD, Threads=$THREADS"

        echo "Nodes: $NODES, Size: $SIZE, Payload: $PAYLOAD, Threads: $THREADS" >> "$OUTPUT_FILE"
        
        # Execute the MPI program with the specified parameters
        srun --mpi=pmix -N "$NODES" -n "$NODES" "$PROGRAM" -s "$SIZE" -r "$PAYLOAD" -t "$THREADS" >> "$OUTPUT_FILE" 2>&1

        echo "------------------------------------------" >> "$OUTPUT_FILE"
      done
    done
  done
done
