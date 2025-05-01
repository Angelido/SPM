# Miniz Code Repository

This repository contains the complete codebase and report for the **Miniz assignment**.

## 📁 Folder Structure

- **`a3-minzip.pdf`** – Original assignment description.  
- **`Report_Nardone_Assignment3.pdf`** – Final report detailing the implementation and results.  
- **`Miniz_Code/`** – Source code, including the `Makefile` to compile the programs.  
- **`Scripts/`** – Bash scripts for running experiments and generating synthetic data.  
- **`Experiments/`** – Includes:
  - Output data from experiments  
  - Python scripts for data analysis and plotting  
  - Final figures used in the report  

## 🛠️ Compilation

To compile the code, navigate to the `Miniz_Code` directory and run:

```bash
make all
```

This command will build the executables required to run the compression and decompression programs.

## 🚀 How to Run the Program

There are two versions of the program:
- **Sequential** (`minizseq`)  
- **Parallel** (`minizpar`)  

The usage syntax is the same for both:

```bash
./<executable> -r {0,1} -C {0,1} [-q {0,1,2}] file1 [file2 ... filek]
./<executable> -r {0,1} -D {0,1} [-q {0,1,2}] file1 [file2 ... filek]
```

### Flags Description:
- `-r`: Enables recursive directory traversal.  
  - `-r 1`: Explore subdirectories recursively  
  - `-r 0`: Only process files in the top-level directory  

- `-C`: Compress the input files.  
  - `-C 0`: Keep original files after compression  
  - `-C 1`: Delete original files after compression  

- `-D`: Decompress the input `.zip` files (mutually exclusive with `-C`).  
  - `-D 0`: Keep compressed files after decompression  
  - `-D 1`: Delete `.zip` files after decompression  

- `-q`: (Optional) Set the verbosity level:  
  - `-q 0`: Silent mode (no output)  
  - `-q 1`: Basic output (default), including error messages  
  - `-q 2`: Verbose output with detailed info (e.g., skipped files)  

- `file1 [file2 ... filek]`: One or more input files or directories to process. At least one is required.

## 📌 Example

```bash
./minizpar -r 1 -C 0 -q 2 data
```

This command runs the **parallel version**, processes the `data` folder recursively, compresses all files (keeping the originals), and uses the most verbose output level.

## 🧵 Configuring the Number of Threads (Parallel Version Only)

In the file `minizpar`, the number of threads is defined on line 97:

```c
int num_threads = 32;
```

To modify the thread count:

### 🔧 Option 1: Edit Source Code
1. Change the number directly in the code.
2. Save and recompile using:

```bash
make all
```

### 🔧 Option 2: Use Environment Variable (Recommended for Flexibility)

1. **Comment out** lines 97 and 101 in `minizpar.c`.
2. Recompile the code:
   ```bash
   make all
   ```
3. Set the number of threads dynamically at runtime:

```bash
export OMP_NUM_THREADS=32
./minizpar -r 1 -C 0 -q 2 data
```

This method allows you to change the thread count without modifying the source code each time.
