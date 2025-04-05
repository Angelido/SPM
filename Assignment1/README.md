# Softmax Code Repository

This repository contains the code and report for the Softmax assignment.

## Folder Structure

- **a1-softmax.pdf**: The assignment track in PDF format.
- **Report_Nardone_Assignment1.pdf**: The assignment report in PDF format.
- **Softmax_Code**: Contains the code for the assignment.
- **scripts**: Contains the main scripts used to perform the experiments.
- **Figures**: Contains the figures generated from the experiments. Two of them are included in the report, and one figure shows the speedup curve.


## How to Use

To use the code, first compile it using `make`.

Then run the program with the following command:

```bash
./softmax_*** K [1]
```

- Replace `***` with one of the following options depending on the implementation you want to use: `plain`, `avx`, or `auto`.
- `K` is a positive integer that specifies the size of the randomly generated array.
- `[1]` is an optional argument; if provided, it will print the computed result to the console.