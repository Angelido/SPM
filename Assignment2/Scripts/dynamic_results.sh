#!/bin/bash

# Definyng the range of X and Y values
X_values=(1 2 4 8 16 32 64)
Y_values=(1 2 4 8 16 32 64)

# Output file
output_file="dynamic_strong_scalability.txt"

# Empty the output file before starting
> "$output_file"

# Loop through each combination of X and Y values
for X in "${X_values[@]}"; do
    for Y in "${Y_values[@]}"; do
        echo "Running for X=$X, Y=$Y" >> "$output_file"
        for i in {1..10}; do
            ./final_version -d -n "$X" -c "$Y" 1-1000 10000-1000000 50000000-100000000 >> "$output_file"
            echo "" >> "$output_file"  
        done
        echo "" >> "$output_file"  
    done
done