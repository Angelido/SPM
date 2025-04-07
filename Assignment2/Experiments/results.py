import pandas as pd
import numpy as np
import re
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
import os



#========= Parse Collatz results =========
def parse_collatz_results(file_path: str) -> pd.DataFrame:
    """
    Function to parse the results of the Collatz benchmark from a file. \n
    Input:
    - file_path: path to the file containing the results \n
    Output:
    - df: DataFrame containing the parsed results
    """
    
    with open(file_path, "r") as file:
        lines = file.readlines()
    
    data = []
    current_entry = {}
    ranges = []
    
    for line in lines:
        line = line.strip()
        
        if not line:  # empty line
            if current_entry:
                current_entry["range"] = ", ".join(ranges)
                data.append(current_entry)
                current_entry = {}
                ranges = []
            continue
        
        if "Dynamic mode:" in line:
            current_entry["dynamic"] = "ON" in line
        elif "Number of threads:" in line:
            current_entry["num_thread"] = int(line.split(": ")[-1])
        elif "Number of tasks (chunk size):" in line:
            current_entry["chunk_size"] = int(line.split(": ")[-1])
        elif "# elapsed time" in line:
            match = re.search(r"([\d\.]+)s", line)
            if match:
                current_entry["time"] = float(match.group(1))
        elif re.match(r"^\d+-\d+$", line):  # range line
            ranges.append(line)
    
    # Add the last entry if it exists
    if current_entry:
        current_entry["range"] = ", ".join(ranges)
        data.append(current_entry)
    
    return pd.DataFrame(data)

import matplotlib.pyplot as plt



#========= Plotting functions strong scaling =========
def plot_time_vs_threads_multi(
    dfs: list[pd.DataFrame],
    chunk_sizes: list[int],
    titles: list[str] = None,
    seq_time: float = None,
    log_x: bool = False,
    log_y: bool = False,
    grid: bool = False,
    save: bool = False,
    save_path: str = None,
) -> None:
    """
    Plots multiple subplots of average time (y-axis) vs number of threads (x-axis)
    for different DataFrames and chunk sizes. \n

    Input:
    - dfs: List of DataFrames with ['num_thread', 'chunk_size', 'time']
    - chunk_sizes: List of chunk sizes to filter and plot (applied to all DataFrames)
    - titles: List of titles for each subplot (same length as dfs)
    - seq_time: Float value to draw a horizontal red dashed line on each subplot
    - log_x: Set x-axis to logarithmic scale (default: False)
    - log_y: Set y-axis to logarithmic scale (default: False)
    - grid: Display grid on plots (default: False)
    """
    num_plots = len(dfs)
    fig, axes = plt.subplots(1, num_plots, figsize=(8 * num_plots, 6), squeeze=False)

    for idx, (df, ax) in enumerate(zip(dfs, axes[0])):
        for chunk_size in chunk_sizes:
            # Filter the dataframe by chunk size
            filtered_df = df[df["chunk_size"] == chunk_size]

            # Sort by num_thread to improve line plotting
            filtered_df = filtered_df.sort_values("num_thread")

            # Plot the curve
            ax.plot(filtered_df["num_thread"], filtered_df["time"],
                    marker="o", linestyle="-", label=f"Chunk size {chunk_size}")

        # Add red dashed line for sequential baseline if provided
        if seq_time is not None:
            ax.axhline(y=seq_time, color="red", linestyle="--", label="Sequential Time")

        # Set title
        if titles and idx < len(titles):
            ax.set_title(titles[idx])

        # Set axis labels
        ax.set_xlabel("Number of Threads")
        ax.set_ylabel("Average Time (s)")

        # Set log scales if requested
        if log_x:
            ax.set_xscale("log", base=2)
            ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: f'{int(x)}')) 

        if log_y:
            ax.set_yscale("log", base=2)
            ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{int(y)}' if y >= 1 else f'{y:.1f}'))


        # Set grid if requested
        if grid:
            ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        ax.legend()

    if save:
        if save_path is None:
            raise ValueError("save_path must be provided if save is True.")
        plt.savefig(save_path)
    
    plt.tight_layout()
    plt.show()
    


#========= Plotting functions speedup =========
def plot_speedup_vs_threads_multi(
    dfs: list[pd.DataFrame],
    chunk_sizes: list[int],
    titles: list[str] = None,
    seq_time: float = None,
    log_x: bool = False,
    log_y: bool = False,
    grid: bool = False,
    save: bool = False,
    save_path: str = None,
) -> None:
    """
    Plots multiple subplots of speedup (y-axis) vs number of threads (x-axis)
    for different DataFrames and chunk sizes. \n

    Speedup is computed as seq_time / T_t, where seq_time is given as input. \n

    Input:
    - dfs: List of DataFrames with ['num_thread', 'chunk_size', 'time']
    - chunk_sizes: List of chunk sizes to filter and plot
    - titles: List of titles for each subplot (same length as dfs)
    - seq_time: Sequential baseline time to compute speedup
    - log_x, log_y: Enable log scale on axes
    - grid: Enable grid on plots
    - save: Save figure to disk
    - save_path: Required if save=True
    """
    if seq_time is None:
        raise ValueError("seq_time must be provided to compute speedup.")

    num_plots = len(dfs)
    fig, axes = plt.subplots(1, num_plots, figsize=(8 * num_plots, 6), squeeze=False)

    for idx, (df, ax) in enumerate(zip(dfs, axes[0])):
        for chunk_size in chunk_sizes:
            df_chunk = df[df["chunk_size"] == chunk_size].copy()
            df_chunk = df_chunk.sort_values("num_thread")

            df_chunk["speedup"] = seq_time / df_chunk["time"]
            ax.plot(df_chunk["num_thread"], df_chunk["speedup"],
                    marker="o", linestyle="-", label=f"Chunk size {chunk_size}")
        
        x_vals = df["num_thread"].values
        ax.plot(x_vals, x_vals, color='r', linestyle='--', label="Ideal Speedup (x=y)")

        # Etichette e opzioni grafiche
        if titles and idx < len(titles):
            ax.set_title(titles[idx])
        ax.set_xlabel("Number of Threads")
        ax.set_ylabel("Speedup")
        
        #Set the log scale if requested
        if log_x:
            ax.set_xscale("log", base=2)
            ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: f'{int(x)}'))

        if log_y:
            ax.set_yscale("log", base=2)
            ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{int(y)}' if y >= 1 else f'{y:.1f}'))
        if grid:
            ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        ax.legend()

    if save:
        if save_path is None:
            raise ValueError("save_path must be provided if save is True.")
        plt.savefig(save_path)

    plt.tight_layout()
    plt.show()




#========= Main =========
if __name__ == "__main__":

    file_path = "Results/dynamic_strong_scaling.txt"  
    df_dynamic = parse_collatz_results(file_path)
    df_aggregato_din = df_dynamic.groupby(["dynamic", "num_thread", "chunk_size"])["time"].mean().reset_index()
    
    file_path = "Results/static_strong_scaling.txt"  
    df_static = parse_collatz_results(file_path)
    df_aggregato_sta = df_static.groupby(["dynamic", "num_thread", "chunk_size"])["time"].mean().reset_index()
    
    plot_time_vs_threads_multi(
    dfs=[df_aggregato_din, df_aggregato_sta],
    chunk_sizes=[4, 16, 32],
    titles=["Dynamic Scheduling Strong Scaling", "Static Scheduling Strong Scaling"],
    seq_time=18, 
    log_x=True,
    log_y=True,
    grid=True,
    save=True,
    save_path="Figures/strong_scaling2.png"
)
    
    plot_speedup_vs_threads_multi(
    dfs=[df_aggregato_din, df_aggregato_sta],
    chunk_sizes=[4, 16, 64],
    titles=["Dynamic Scheduling Speedup", "Static Scheduling Speedup"],
    seq_time=18.4,
    log_x=True,
    log_y=True,
    grid=True,
    save=True,
    save_path="Figures/speedup_comparison2.png"
)
