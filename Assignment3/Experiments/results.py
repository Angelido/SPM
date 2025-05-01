import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter



#========= Plot Speedup vs Threads =========
def plot_speedup_multi_inputs_subplots(
    df: pd.DataFrame,
    input_names: list[str],
    log_x: bool = False,
    log_y: bool = False,
    grid: bool = False,
    save: bool = False,
    save_path: str = None,
) -> None:
    """
    Shows multiple subplots of speedup vs threads for different inputs of minizpar.

    Inputs:
    - df: DataFrame with columns ['Program', 'Operation', 'Input', 'Threads', 'Time']
    - input_names: Name of the input to plot (one per subplot)
    - log_x: Logarithmic scale on the x-axis
    - log_y: Logarithmic scale on the y-axis
    - grid: Show grid in the plots
    - save: Save the figure to file
    - save_path: Path to save if save=True
    """
    num_inputs = len(input_names)
    fig, axes = plt.subplots(1, num_inputs, figsize=(6 * num_inputs, 5), squeeze=False)

    # Loop through each input name and create a subplot
    for idx, input_name in enumerate(input_names):
        ax = axes[0][idx]

        # Filter DataFrame for the current input
        par_df = df[
            (df['Program'] == 'minizpar') &
            (df['Operation'] == 'C') &
            (df['Input'] == input_name)
        ]

        seq_df = df[
            (df['Program'] == 'minizseq') &
            (df['Operation'] == 'C') &
            (df['Input'] == input_name)
        ]

        if seq_df.empty or par_df.empty:
            print(f"Warning: No data for input '{input_name}' in either minizpar or minizseq.")
            continue

        # Calculate speedup
        seq_time = seq_df['Time'].values[0]
        par_df = par_df.copy()
        par_df['Speedup'] = seq_time / par_df['Time']
        par_df = par_df.sort_values('Threads')

        # Plot the speedup
        ax.plot(
            par_df['Threads'],
            par_df['Speedup'],
            marker='o',
            linestyle='-',
            label=f'{input_name} Speedup'
        )

        # Plot the ideal speedup line
        x_vals = par_df["Threads"].values
        ax.plot(x_vals, x_vals, color='r', linestyle='--', label="Ideal Speedup (x=y)")

        ax.set_title(f"{input_name} Speedup vs Threads")
        ax.set_xlabel('Number of Threads')
        ax.set_ylabel('Speedup')

        # Set log scale if requested
        if log_x:
            ax.set_xscale("log", base=2)
            ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: f'{int(x)}' if x >= 1 else f'{x:.1f}'))
        if log_y:
            ax.set_yscale("log", base=2)
            ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{int(y)}' if y >= 1 else f'{y:.1f}'))
        # Set grid if requested
        if grid:
            ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        ax.legend()
        
    plt.tight_layout()

    # Save the figure if requested
    if save:
        if save_path is None:
            raise ValueError("Save_path must be provided if save is True.")
        plt.savefig(save_path, bbox_inches='tight')

    plt.show()
    


#========= Plot Execution Time vs Threads =========
def plot_execution_time_multi_inputs_subplots(
    df: pd.DataFrame,
    input_names: list[str],
    log_x: bool = False,
    log_y: bool = False,
    grid: bool = False,
    save: bool = False,
    save_path: str = None,
) -> None:
    """
    Shows multiple subplots of execution time vs threads for different inputs of minizpar.

    Inputs:
    - df: DataFrame with columns ['Program', 'Operation', 'Input', 'Threads', 'Time']
    - input_names: Name of the input to plot (one per subplot)
    - log_x: Logarithmic scale on the x-axis
    - log_y: Logarithmic scale on the y-axis
    - grid: Show grid in the plots
    - save: Save the figure to file
    - save_path: Path to save if save=True
    """
    num_inputs = len(input_names)
    fig, axes = plt.subplots(1, num_inputs, figsize=(6 * num_inputs, 5), squeeze=False)

    # Loop through each input name and create a subplot
    for idx, input_name in enumerate(input_names):
        ax = axes[0][idx]

        # Filter DataFrame for the current input
        par_df = df[
            (df['Program'] == 'minizpar') &
            (df['Operation'] == 'C') &
            (df['Input'] == input_name)
        ]

        seq_df = df[
            (df['Program'] == 'minizseq') &
            (df['Operation'] == 'C') &
            (df['Input'] == input_name)
        ]

        if seq_df.empty or par_df.empty:
            print(f"Warning: No data for input '{input_name}' in either minizpar or minizseq.")
            continue

        # Get sequential execution time
        seq_time = seq_df['Time'].values[0]
        par_df = par_df.copy()
        par_df = par_df.sort_values('Threads')

        # Plot the execution time
        ax.plot(
            par_df['Threads'],
            par_df['Time'],
            marker='o',
            linestyle='-',
            label=f'{input_name} Execution Time'
        )

        # Plot the horizontal line for sequential time
        ax.axhline(y=seq_time, color='r', linestyle='--', label="Sequential Time")

        ax.set_title(f"{input_name} Execution Time vs Threads")
        ax.set_xlabel('Number of Threads')
        ax.set_ylabel('Execution Time (seconds)')

        # Set log scale if requested
        if log_x:
            ax.set_xscale("log", base=2)
            ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: f'{int(x)}' if x >= 1 else f'{x:.1f}'))
        if log_y:
            ax.set_yscale("log", base=2)
            ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{int(y)}' if y >= 1 else f'{y:.1f}'))
        # Set grid if requested
        if grid:
            ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        ax.legend()

    plt.tight_layout()

    # Save the figure if requested
    if save:
        if save_path is None:
            raise ValueError("Save_path must be provided if save is True.")
        plt.savefig(save_path, bbox_inches='tight')

    plt.show()



#========= Main =========
if __name__ == "__main__":
    
    # Read the CSV file
    df = pd.read_csv("results.csv", skip_blank_lines=True)

    # Clean the DataFrame
    df['Time'] = df['Output'].str.extract(r'(\d+\.\d+)').astype(float)
    df['Input'] = df['Command'].str.split().str[-1]
    df['Operation'] = df['Command'].str.split().apply(lambda x: 'C' if len(x) > 2 and x[2] == '-C\\' else ('D' if len(x) > 2 and x[2] == '-D\\' else ''))

    # Group by Program, Operation, Input, and Threads
    grouped = df.groupby(['Program', 'Operation', 'Input', 'Threads'])['Time'].mean().reset_index()
    print(grouped)
    
    # Plot speedup for multiple inputs
    plot_speedup_multi_inputs_subplots(
    df=grouped,
    input_names=['big_files', 'small_files', 'nested_files'],
    log_x=True,
    log_y=False,
    grid=True,
    save=True,
    save_path="Figures/speedup_plot.png"
    )
    
    # Plot execution time for multiple inputs
    plot_execution_time_multi_inputs_subplots(
    grouped,
    input_names=['big_files', 'small_files', 'nested_files'],
    log_x=False,    
    log_y=True,    
    grid=True,      
    save=True,
    save_path="Figures/execution_time_plot.png"      
    )

