#!/bin/bash

# Script for generating small random files
# Usage: ./generate_files.sh

DIR="small_files"
TARGET_SIZE=$((400 * 1024 * 1024))  # 400 MB in bytes
MIN_SIZE=$((100 * 1024))            # 100KB
MAX_SIZE=$((1024 * 1024))           # 1MB
TOTAL_SIZE=0
COUNTER=1

# Create the directory if it doesn't exist
mkdir -p "$DIR" || { echo "Errore: impossibile creare la directory $DIR"; exit 1; }

echo "Generazione file casuali in corso..."
echo "Dimensione target: $((TARGET_SIZE / (1024*1024))) MB"

while [ $TOTAL_SIZE -lt $TARGET_SIZE ]; do
    # Compute random size
    SIZE=$(( (RANDOM % (MAX_SIZE - MIN_SIZE + 1) + MIN_SIZE ))
    
    # Generate random file
    FILENAME="${DIR}/small_$(printf "%03d" $COUNTER).dat"
    dd if=/dev/urandom of="$FILENAME" bs=1 count=$SIZE status=none 2>/dev/null
    
    # Update total size
    TOTAL_SIZE=$((TOTAL_SIZE + SIZE))
    COUNTER=$((COUNTER + 1))
    
    # Print progress
    echo -ne "Generati $COUNTER file ($((TOTAL_SIZE / (1024*1024))) MB)\r"
done

echo -e "\nCompletato!"
echo "File generati: $((COUNTER-1))"
echo "Dimensione totale: $((TOTAL_SIZE / (1024*1024))) MB"