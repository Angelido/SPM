#!/bin/bash

# Nome del file di output
OUTPUT_FILE="auto_node_results.txt"

# Pulisce il file di output
echo "K Time(ms)" > "$OUTPUT_FILE"

# Array con i valori di K da testare
K_VALUES=(100 200 300 400 500 600 700 800 900 1000)

# Loop sui valori di K
for K in "${K_VALUES[@]}"; do
    for i in {1..10}; do
        # Esegue il programma e salva il tempo di esecuzione
        TIME_MS=$(./softmax_auto "$K" | grep "softime_auto" | awk '{print substr($5, 1, length($5)-1)}')
        echo "$K $TIME_MS" >> "$OUTPUT_FILE"
    done
done

echo "Test completato. I risultati sono in $OUTPUT_FILE"
