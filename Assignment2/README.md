# Collatz Code Repository

This repository contains the code and report for the **Collatz assignment**.

## 📁 Folder Structure

- **`a2-collatz.pdf`**: The original assignment description in PDF format.  
- **`Report_Nardone_Assignment2.pdf`**: Final report for the assignment.  
- **`Collatz_Code/`**: Source code for the assignment, including the Makefile to compile the programs.  
- **`Scripts/`**: Python scripts used to perform and manage the experiments.  
- **`Experiments/`**: Contains:
  - Output results from the experiments
  - Python scripts for processing and plotting data
  - Final figures included in the report  

## 🛠️ Compilation

To compile the code, navigate to the `Collatz_Code` folder and run:

```bash
make all
```

This will generate the executables needed to run the experiments.


## 🚀 How to Run the Program

There are three versions of the program: sequential, parallel with static scheduling, and parallel with dynamic scheduling.

### 🔹 Sequential Version

```bash
./sequential_collatz range1_start-range1_end [range2_start-range2_end ...]
```

- Each range must be defined with two **positive integers**, and `rangeX_end > rangeX_start`.
- At least **one range** is required.
- You can provide **multiple ranges**, separated by spaces.

### 🔹 Parallel Version – Static Scheduling

```bash
./parallel_collatz [-n N] [-c C] range1_start-range1_end [range2_start-range2_end ...]
```

- `-n N`: (Optional) Number of threads. Default is `16`.
- `-c C`: (Optional) Chunk size. Default is `1`.
- Ranges follow the same rules as above.


### 🔹 Parallel Version – Dynamic Scheduling

```bash
./parallel_collatz -d [-n N] [-c C] range1_start-range1_end [range2_start-range2_end ...]
```

- Same as the static version, but with the `-d` flag to enable **dynamic scheduling**.

---

## 📌 Example

```bash
./parallel_collatz -d -n 8 -c 2 1-1000 2000-3000
```

This command runs the **parallel dynamic version** using **8 threads**, a **chunk size of 2**, and computes the Collatz steps for the numbers in the ranges `1–1000` and `2000–3000`.

