# MergeSort Code Repository

This repository contains the full implementation and final report for the **MergeSort assignment**.

## 📁 Folder Structure

* **`a4-sort.pdf`** – Original assignment description.
* **`Report_Nardone_Assignment4.pdf`** – Final report including implementation details and experimental results.
* **`MergeSort_Code/`** – Source code and `Makefile` for compiling the programs.
* **`Scripts/`** – Bash scripts for running experiments and generating synthetic data.
* **`Experiments/`** – Includes:

  * Raw output data from experiments
  * Python scripts for analysis and plotting
  * Final figures used in the report

## 🛠️ Compilation

To compile the code, navigate to the `MergeSort_Code` directory and run:

```bash
make all
```

This will build the required executables for the sequential, parallel, and distributed versions of the program.

## 🚀 How to Run the Program

There are three implementations available:

* **Sequential version** (`mergesort_seq`)
* **Parallel version (single node, using FastFlow)** (`mergesort_par`)
* **Distributed version (multi-node with MPI)** (`mergesort_mpi`)

Use the following syntax to run the programs locally:

```bash
./mergesort_seq -s SIZE -r RPAYLOAD 
./mergesort_par -s SIZE -r RPAYLOAD -t N_THREADS
mpirun -np N_NODES ./mergesort_mpi -s SIZE -r RPAYLOAD -t N_THREADS
```

Or use this syntax when running on a server with multiple nodes:

```bash
srun ./mergesort_seq -s SIZE -r RPAYLOAD 
srun ./mergesort_par -s SIZE -r RPAYLOAD -t N_THREADS
srun --mpi=pmix -N N_NODES -n N_PROCESS ./mergesort_mpi -s SIZE -r RPAYLOAD -t N_THREADS
```

### Flags Description:

* `-s SIZE`: Size of the vector of Records (default = 1M).
* `-r RPAYLOAD`: Payload size in bytes for each Record (default = 32).
* `-t N_THREADS`: Number of threads to use per node in the FastFlow parallel version (default = 8).
* `-N N_NODES`: Number of nodes to use in the distributed MPI version.
* `-n N_PROCESS`: Number of MPI processes to run in the distributed version.

It is generally recommended to set the number of nodes (`-N`) equal to the number of MPI processes (`-n`) in the distributed version.

The `-s SIZE` flag also accepts suffixes like `K` for thousands and `M` for millions (e.g., `10M` = 10 million records).

## 📌 Example Usage

```bash
srun ./mergesort_par -s 10M -r 128 -t 16
```

This command runs the **parallel version** on a single node, sorting a vector of 10 million records, each with a 128-byte payload, using 16 threads.

```bash
srun --mpi=pmix -N 8 -n 8 ./mergesort_mpi -s 100M -r 64 -t 32
```

This command runs the **distributed version**, sorting a vector of 100 million records with 64-byte payloads, using 32 threads per node on 8 nodes, with 8 MPI processes.


