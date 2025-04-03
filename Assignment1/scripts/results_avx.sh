#!/bin/bash

# File di output
OUTPUT_FILE="avx.txt"

# Svuota il file di output se esiste
echo "" > "$OUTPUT_FILE"

# Valori di K
declare -a K_VALUES=(100 200 600 1000)

# Esegue il comando per ogni valore di K e salva l'output
for K in "${K_VALUES[@]}"; do
    echo "\nK=$K" >> "$OUTPUT_FILE"
    ./softmax_avx "$K" 1 2>> "$OUTPUT_FILE"
done

echo "Risultati salvati in $OUTPUT_FILE"