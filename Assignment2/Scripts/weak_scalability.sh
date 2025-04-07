#!/bin/bash

# Defining the range of X values
X_values=(1 2 4 8 16 32)

# Output file
output_file="weak_scalability.txt"

# Empty the output file before starting
> weak_scalability.txt

# Loop through each combination of X values
for X in "${X_values[@]}"; do
    for i in {1..10}; do
        
        ./final_version -n $X -c 64 1-${X}0000000 >> weak_scalability.txt
        echo -e "\n" >> "$output_file"

        ./final_version -d -n $X -c 64 1-${X}0000000 >> weak_scalability.txt
        echo -e "\n" >> "$output_file"
    done
done