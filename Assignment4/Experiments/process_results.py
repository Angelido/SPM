import pandas as pd
import numpy as np
import re
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker



#================== parse_sequential_results_txt ==================
def parse_sequential_results_txt(filepath):
    """
    Parses a benchmark results .txt file and extracts Size, Range, and Computation Time.

    Input:
    - filepath (str): Path to the .txt file.

    Return:
    - pd.DataFrame: DataFrame with columns ['Size', 'Range', 'Time']
    """
    
    # Regex patterns to match the relevant lines in the file
    size_pattern = re.compile(r'Size:\s*(\d+)([KM])')
    range_pattern = re.compile(r'Range:\s*(\d+)')
    time_pattern = re.compile(r'# elapsed time \(sequential_mergesort\):\s*([\d\.]+)s')

    # Lists to store the extracted values
    data = {
        'Size': [],
        'Payload': [],
        'Time': []
    }

    # Open and read the file
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Split it into blocks based on "Esecuzione"
    # (Execution in Italian)
    blocks = content.split("Esecuzione")
    for block in blocks:
        size_match = size_pattern.search(block)
        range_match = range_pattern.search(block)
        time_match = time_pattern.search(block)

        # If all matches are found, extract the values
        if size_match and range_match and time_match:
            size_val = int(size_match.group(1))
            size_unit = size_match.group(2)
            size = size_val * (1_000 if size_unit == 'K' else 1_000_000)

            range_val = int(range_match.group(1))
            time_sec = float(time_match.group(1))

            data['Size'].append(size)
            data['Payload'].append(range_val)
            data['Time'].append(time_sec)

    return pd.DataFrame(data)


#================== parse_parallel_results_txt ==================
def parse_parallel_results_txt(filepath):
    """
    Parses a benchmark result text file from the parallel version of mergesort.

    The function extracts:
    - Size (in integers, supports 'K' and 'M' units)
    - Range (record payload)
    - Threads used
    - Execution time (in seconds)

    Input:
    - filepath (str): path to the .txt file to parse

    Return:
    - pd.DataFrame: DataFrame containing columns: ['Size', 'Range', 'Threads', 'Time']
    """
    # Lists to store the extracted values
    data = {
        'Size': [],
        'Payload': [],
        'Threads': [],
        'Time': []
    }

    # Open and read the file
    with open(filepath, 'r') as file:
        lines = file.readlines()

    # Regular expressions for matching patterns
    run_pattern = re.compile(r'Running N=(\d+)([KM]), P=\d+, T=(\d+)')
    payload_pattern = re.compile(r'Record payload:\s+(\d+)')
    time_pattern = re.compile(r'# elapsed time .*?: ([\d.]+)s')

    size = range_val = threads = time_sec = None

    for line in lines:
        line = line.strip()

        # Match size, unit and threads
        run_match = run_pattern.search(line)
        if run_match:
            size_val = int(run_match.group(1))
            size_unit = run_match.group(2)
            threads = int(run_match.group(3))
            size = size_val * (1_000 if size_unit == 'K' else 1_000_000)
            continue

        # Match range (record payload)
        payload_match = payload_pattern.search(line)
        if payload_match:
            range_val = int(payload_match.group(1))
            continue

        # Match execution time
        time_match = time_pattern.search(line)
        if time_match:
            time_sec = float(time_match.group(1))
            # Append a new row to the data
            data['Size'].append(size)
            data['Payload'].append(range_val)
            data['Threads'].append(threads)
            data['Time'].append(time_sec)

    # Create and return the DataFrame
    return pd.DataFrame(data)


#================== parse_distributed_results_txt ==================
def parse_distributed_results_txt(filepath):
    """
    Parses a distributed MergeSort benchmark text file and extracts relevant performance data.

    Extracts:
    - Nodes (from "MPI numbero of nodes", defaults to 1 if not found)
    - Size (from "Array size")
    - Payload (from "Record payload")
    - Threads (from "N° cores according to FastFlow")
    - Time (from "Tempo totale (escl. init)" or "# elapsed time")

    Input:
    - filepath (str): path to the benchmark .txt file

    Return:
    - pd.DataFrame: DataFrame with columns ['Nodes', 'Size', 'Payload', 'Threads', 'Time']
    """
    data = {
        'Nodes': [],
        'Size': [],
        'Payload': [],
        'Threads': [],
        'Time': []
    }

    # Regex patterns
    payload_pattern = re.compile(r'Record payload:\s*(\d+)')
    size_pattern = re.compile(r'Array size:\s*(\d+)')
    threads_pattern = re.compile(r'N° cores according to FastFlow:\s*(\d+)')
    nodes_pattern = re.compile(r'MPI numbero of nodes:\s*(\d+)')
    time_pattern_1 = re.compile(r'Tempo totale \(escl\. init\):\s*([\d.]+)')
    time_pattern_2 = re.compile(r'# elapsed time.*?:\s*([\d.]+)s')

    # Temporary values
    payload = size = threads = time = None
    nodes = 1  # Default to 1 node

    with open(filepath, 'r') as file:
        for line in file:
            line = line.strip()

            if line.startswith('Nodes:'):
                # New block: reset variables and set default nodes=1
                payload = size = threads = time = None
                nodes = 1  # reset default

            if m := payload_pattern.match(line):
                payload = int(m.group(1))
            elif m := size_pattern.match(line):
                size = int(m.group(1))
            elif m := threads_pattern.match(line):
                threads = int(m.group(1))
            elif m := nodes_pattern.match(line):
                nodes = int(m.group(1))
            elif m := time_pattern_1.match(line) or time_pattern_2.match(line):
                time = float(m.group(1))

                # Save current block if all values are present
                if all(v is not None for v in [payload, size, threads, time]):
                    data['Nodes'].append(nodes)
                    data['Size'].append(size)
                    data['Payload'].append(payload)
                    data['Threads'].append(threads)
                    data['Time'].append(time)

                # Prepare for next block
                payload = size = threads = time = None
                nodes = 1  # Reset default again

    return pd.DataFrame(data)



# ================== aggregate_by_mean ==================
def aggregate_by_mean(df, group_cols, time_col='Time'):
    """
    Merge the DataFrame by calculating the mean of the specified time column for each group defined by the group_cols.
    
    Input:
    - df (pd.DataFrame): DataFrame to aggregate
    - group_cols (list of str): columns to group by (e.g., ['Size', 'Range'])
    - time_col (str): name of the column containing time values (default is 'Time')

    Return:
    - pd.DataFrame: DataFrame aggretated
    """
    
    aggregated_df = df.groupby(group_cols, as_index=False)[time_col].mean()
    return aggregated_df



# ================== plot_parallel_speedup_by_payload ==================
def plot_parallel_speedup_by_payload(
    parallel_df,
    sequential_df,
    size_filter,
    time_col='Time',
    thread_col='Threads',
    payload_col='Payload',
    size_col='Size',
    label='Speedup',
    show_grid=True,
    log_x=False,
    log_y=False,
    xlabel='Number of Threads',
    ylabel='Speedup',
    show_legend=True,
    ideal_speedup=False,
    save_fig=False,
    fig_name='speedup_plot.png'
):
    """
    Plot speedup curves by payload, comparing parallel times to sequential times at fixed input size.
    Also plots the ideal linear speedup line y=x.
    
    Input:
    - parallel_df: DataFrame containing parallel or distributed execution results.
    - sequential_df: DataFrame containing sequential execution results.
    - size_filter: The size value (e.g., 100000000) to filter on.
    - time_col: Column name for execution time.
    - thread_col: Column name for number of threads.
    - payload_col: Column name for record payload.
    - size_col: Column name for size of the input.
    - label: Title of the plot.
    - show_grid: Whether to show a grid on the plot.
    - log_x: Set x-axis to logarithmic scale.
    - log_y: Set y-axis to logarithmic scale.
    - xlabel: Label for the x-axis.
    - ylabel: Label for the y-axis.
    - show_legend: Whether to display the legend.
    - ideal_speedup: Whether to plot the ideal linear speedup line.
    - save_fig: Whether to save the figure to a file.
    - fig_name: Name of the file to save the figure.
    """
    plt.figure(figsize=(8, 5))

    # Filter data for selected size
    par_filtered = parallel_df[parallel_df[size_col] == size_filter]
    seq_filtered = sequential_df[sequential_df[size_col] == size_filter]

    if par_filtered.empty or seq_filtered.empty:
        print(f"No data found for size {size_filter}")
        return

    max_threads = par_filtered[thread_col].max()
    threads_range = np.arange(1, max_threads+1)

    for payload in sorted(par_filtered[payload_col].unique()):
        par_payload = par_filtered[par_filtered[payload_col] == payload]
        seq_payload = seq_filtered[seq_filtered[payload_col] == payload]

        if seq_payload.empty:
            continue

        seq_time = seq_payload[time_col].mean()

        grouped = par_payload.groupby(thread_col)[time_col].mean().reset_index()
        grouped['Speedup'] = seq_time / grouped[time_col]

        plt.plot(
            grouped[thread_col],
            grouped['Speedup'],
            marker='o',
            label=f'Payload {payload}'
        )

    # Plot ideal linear speedup line: y = x
    if ideal_speedup:
        plt.plot(
            threads_range,
            threads_range,
            linestyle='--',
            color='red',
            label='Ideal speedup (y=x)'
        )

    if show_grid:
        plt.grid(True, linestyle='--', alpha=0.6)
    if log_x:
        plt.xscale('log', base=2)
        # Show only major ticks on the x-axis
        plt.gca().xaxis.set_major_locator(ticker.LogLocator(base=2, subs=(1.0,), numticks=10))
        plt.gca().xaxis.set_minor_locator(ticker.NullLocator())
        plt.gca().xaxis.set_major_formatter(ticker.ScalarFormatter())
    if log_y:
        plt.yscale('log')

    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(label)
    if show_legend:
        plt.legend()
    plt.tight_layout()
    
    if save_fig:
        fig_path = "Figures" + f"/{fig_name}"
        plt.savefig(fig_path, dpi=300)
        print(f"Figure saved as {fig_name}")
        
    plt.show()
    


# ================== plot_parallel_efficiency_by_payload ==================
def plot_parallel_efficiency_by_payload(
    parallel_df,
    sequential_df,
    size_filter,
    time_col='Time',
    thread_col='Threads',
    payload_col='Payload',
    size_col='Size',
    node_col='Nodes',
    label='Parallel Efficiency (1 Node)',
    show_grid=True,
    log_x=False,
    log_y=False,
    xlabel='Number of Threads',
    ylabel='Efficiency',
    show_legend=True,
    save_fig=False,
    fig_name='parallel_efficiency_plot.png'
):
    """
    Plot efficiency curves by payload for parallel runs on a single node,
    comparing against sequential execution at a fixed input size.
    
    Input:
    - parallel_df: DataFrame containing parallel results (filtered for Nodes == 1).
    - sequential_df: DataFrame with sequential results.
    - size_filter: Integer, the input size to filter on.
    - time_col: Column with execution time.
    - thread_col: Column with number of threads.
    - payload_col: Column with record size.
    - size_col: Column with input size.
    - node_col: Column with number of nodes (default 'Nodes').
    - label: Title of the plot.
    - show_grid: Whether to display grid.
    - log_x: Use logarithmic scale on x-axis.
    - log_y: Use logarithmic scale on y-axis.
    - xlabel: Label for x-axis.
    - ylabel: Label for y-axis.
    - show_legend: Whether to display legend.
    """
    import matplotlib.pyplot as plt
    import numpy as np
    import matplotlib.ticker as ticker

    plt.figure(figsize=(8, 5))

    # Filter data for the specified size
    par_filtered = parallel_df[
        parallel_df[size_col] == size_filter
    ]
    seq_filtered = sequential_df[sequential_df[size_col] == size_filter]

    if par_filtered.empty or seq_filtered.empty:
        print(f"No data found for size {size_filter}")
        return

    max_threads = par_filtered[thread_col].max()

    for payload in sorted(par_filtered[payload_col].unique()):
        par_payload = par_filtered[par_filtered[payload_col] == payload]
        seq_payload = seq_filtered[seq_filtered[payload_col] == payload]

        if seq_payload.empty:
            continue

        seq_time = seq_payload[time_col].mean()

        grouped = par_payload.groupby(thread_col)[time_col].mean().reset_index()
        grouped['Efficiency'] = seq_time / (grouped[time_col] * grouped[thread_col])

        plt.plot(
            grouped[thread_col],
            grouped['Efficiency'],
            marker='o',
            label=f'Payload {payload}'
        )

    if show_grid:
        plt.grid(True, linestyle='--', alpha=0.6)
    if log_x:
        plt.xscale('log', base=2)
        plt.gca().xaxis.set_major_locator(ticker.LogLocator(base=2, subs=(1.0,), numticks=10))
        plt.gca().xaxis.set_minor_locator(ticker.NullLocator())
        plt.gca().xaxis.set_major_formatter(ticker.ScalarFormatter())
    if log_y:
        plt.yscale('log')

    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(label)
    if show_legend:
        plt.legend()
    plt.tight_layout()
    
    if save_fig:
        fig_path = "Figures" + f"/{fig_name}"
        plt.savefig(fig_path, dpi=300)
        print(f"Figure saved as {fig_name}")
    
    plt.show()
    


# ================== plot_strong_scalability_by_payload ==================
def plot_strong_scalability_by_payload(
    distributed_df,
    size_filter,
    fixed_threads=16,
    time_col='Time',
    nodes_col='Nodes',
    thread_col='Threads',
    payload_col='Payload',
    size_col='Size',
    label='Strong Scalability (Distributed)',
    show_grid=True,
    log_x=False,
    log_y=False,
    xlabel='Number of Nodes',
    ylabel='Speedup (T1 / Tp)',
    show_legend=True,
    ideal_speedup=False,
    save_fig=False,
    fig_name='strong_scalability_plot.png'
):
    """
    Plot strong scalability curves by payload using distributed execution only.
    The reference time T(1) is the time with 1 node and fixed_threads.

    Input:
    - distributed_df: DataFrame with distributed execution results.
    - size_filter: Fixed input size for the test (problem size remains constant).
    - fixed_threads: Number of threads per node.
    - time_col: Column for execution time.
    - nodes_col: Column for number of nodes.
    - thread_col: Column for number of threads.
    - payload_col: Column for record payload.
    - size_col: Column for size of input.
    - label: Title of the plot.
    - show_grid: Show grid on plot.
    - log_x: Log scale on x-axis.
    - log_y: Log scale on y-axis.
    - xlabel, ylabel: Axis labels.
    - show_legend: Whether to show legend.
    - ideal_speedup: Whether to plot the ideal line y = x.
    - save_fig: Whether to save the plot.
    - fig_name: File name for saving.
    """
    plt.figure(figsize=(8, 5))

    # Filter only distributed runs with the fixed size and thread count
    dist_filtered = distributed_df[
        (distributed_df[size_col] == size_filter) &
        (distributed_df[thread_col] == fixed_threads)
    ]

    if dist_filtered.empty:
        print(f"No distributed data found for size {size_filter} and threads {fixed_threads}")
        return

    max_nodes = dist_filtered[nodes_col].max()
    nodes_range = np.arange(1, max_nodes + 1)

    for payload in sorted(dist_filtered[payload_col].unique()):
        data_payload = dist_filtered[dist_filtered[payload_col] == payload]

        base_time_df = data_payload[data_payload[nodes_col] == 1]
        if base_time_df.empty:
            continue  # Cannot compute speedup without T(1)

        T1 = base_time_df[time_col].mean()

        grouped = data_payload.groupby(nodes_col)[time_col].mean().reset_index()
        grouped = grouped[grouped[nodes_col] >= 1]  # ensure node >= 1
        grouped['Speedup'] = T1 / grouped[time_col]

        plt.plot(
            grouped[nodes_col],
            grouped['Speedup'],
            marker='o',
            label=f'Payload {payload}'
        )

    if ideal_speedup:
        plt.plot(
            nodes_range,
            nodes_range,
            linestyle='--',
            color='red',
            label='Ideal speedup (linear)'
        )

    if show_grid:
        plt.grid(True, linestyle='--', alpha=0.6)
    if log_x:
        plt.xscale('log', base=2)
        plt.gca().xaxis.set_major_locator(ticker.LogLocator(base=2, subs=(1.0,), numticks=10))
        plt.gca().xaxis.set_minor_locator(ticker.NullLocator())
        plt.gca().xaxis.set_major_formatter(ticker.ScalarFormatter())
    if log_y:
        plt.yscale('log')

    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(label)
    if show_legend:
        plt.legend()
    plt.tight_layout()
    if save_fig:
        fig_path = "Figures" + f"/{fig_name}"
        plt.savefig(fig_path, dpi=300)
        print(f"Figure saved as {fig_name}")
        
    plt.show()
    

# ================== plot_weak_scalability_by_payload ==================
def plot_weak_scalability_by_payload(
    distributed_df,
    base_size,
    fixed_threads=16,
    time_col='Time',
    nodes_col='Nodes',
    thread_col='Threads',
    payload_col='Payload',
    size_col='Size',
    label='Weak Scalability (Distributed)',
    show_grid=True,
    log_x=False,
    log_y=False,
    xlabel='Number of Nodes\n(with Problem Size in Millions)',
    ylabel='Normalized Time (Tp / T1)',
    show_legend=True,
    ideal_line=False,
    save_fig=False,
    fig_name='weak_scalability_plot.png'
):
    """
    Plot weak scalability: problem size grows with number of nodes.
    For each payload, we start from 1 node and base_size, then double nodes and size.

    Input:
    - distributed_df: DataFrame with distributed execution results.
    - base_size: Input size for 1 node (e.g., 1_000_000).
    - fixed_threads: Number of threads per node.
    - time_col: Execution time column.
    - nodes_col: Number of nodes.
    - thread_col: Number of threads per node.
    - payload_col: Column representing record payload.
    - size_col: Total input size (should scale with nodes).
    - label: Title of the plot.
    - show_grid: Whether to show grid.
    - log_x, log_y: Log scale for axes.
    - xlabel, ylabel: Axis labels.
    - show_legend: Show legend.
    - ideal_line: Show ideal constant time line.
    - save_fig: Save to file.
    - fig_name: File name.
    """
    plt.figure(figsize=(8, 5))

    # Filter distributed runs with fixed threads
    dist_filtered = distributed_df[distributed_df[thread_col] == fixed_threads]

    if dist_filtered.empty:
        print(f"No data found with threads={fixed_threads}")
        return

    max_nodes = dist_filtered[nodes_col].max()
    powers_of_two = [2 ** i for i in range(int(np.log2(max_nodes)) + 1)]
    valid_nodes = [n for n in powers_of_two if n <= max_nodes]

    for payload in sorted(dist_filtered[payload_col].unique()):
        x_nodes = []
        y_times = []

        for nodes in valid_nodes:
            current_size = base_size * nodes
            subset = dist_filtered[
                (dist_filtered[nodes_col] == nodes) &
                (dist_filtered[size_col] == current_size) &
                (dist_filtered[payload_col] == payload)
            ]

            if subset.empty:
                continue

            avg_time = subset[time_col].mean()
            x_nodes.append(nodes)
            y_times.append(avg_time)

        if not x_nodes or x_nodes[0] != 1:
            continue  # We need T1 for normalization

        T1 = y_times[0]
        normalized_times = [t / T1 for t in y_times]

        plt.plot(
            x_nodes,
            normalized_times,
            marker='o',
            label=f'Payload {payload}'
        )

    if ideal_line:
        plt.hlines(1.0, min(valid_nodes), max(valid_nodes), linestyles='--', colors='red', label='Ideal (constant time)')

    if show_grid:
        plt.grid(True, linestyle='--', alpha=0.6)
    if log_x:
        plt.xscale('log', base=2)
        plt.gca().xaxis.set_major_locator(ticker.LogLocator(base=2))
        plt.gca().xaxis.set_minor_locator(ticker.NullLocator())
        plt.gca().xaxis.set_major_formatter(ticker.ScalarFormatter())
    if log_y:
        plt.yscale('log')

    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(label)
    if show_legend:
        plt.legend()
    plt.tight_layout()
    xtick_labels = [f"{n}\n{base_size * n // 1_000_000}M" for n in valid_nodes]
    plt.xticks(valid_nodes, labels=xtick_labels)
    if save_fig:
        fig_path = "Figures" + f"/{fig_name}"
        plt.savefig(fig_path, dpi=300)
        print(f"Figure saved as {fig_name}")

    plt.show()



# ================== main ==================
if __name__ == "__main__":
    
    # Parse the results from the text files
    sequential_df = parse_sequential_results_txt('Results/seq_result.txt')
    parallel_df = parse_parallel_results_txt('Results/par_result.txt')
    distributed_df = parse_distributed_results_txt('Results/mpi_result.txt')

    # Aggregate by mean
    sequential_df = aggregate_by_mean(sequential_df, ['Size', 'Payload'])
    parallel_df = aggregate_by_mean(parallel_df, ['Size', 'Payload', 'Threads'])
    distributed_df = aggregate_by_mean(distributed_df, ['Nodes', 'Size', 'Payload', 'Threads'])

    #================== Parallel Plotting ==================
    plot_parallel_speedup_by_payload(
        parallel_df=parallel_df,
        sequential_df=sequential_df,
        size_filter=10000000,  # 10M
        thread_col='Threads',
        payload_col='Payload',
        size_col='Size',
        label='Parallel Speedup over Sequential (Size: 10M)',
        show_grid=True,
        log_x=True,
        save_fig=True,
        fig_name='parallel_speedup10M.png',
    )
    
    plot_parallel_speedup_by_payload(
        parallel_df=parallel_df,
        sequential_df=sequential_df,
        size_filter=100000000,  # 100M
        thread_col='Threads',
        payload_col='Payload',
        size_col='Size',
        label='Parallel Speedup over Sequential (Size: 100M)',
        show_grid=True,
        log_x=True,
        save_fig=True,
        fig_name='parallel_speedup100M.png',
    )

    plot_parallel_efficiency_by_payload(
        parallel_df=parallel_df,
        sequential_df=sequential_df,
        size_filter=100000000,
        label='Parallel Efficiency over Sequential (Size: 100M)',
        save_fig=True,
        fig_name='parallel_efficiency.png',
    )
    
    #================== Distributed Plotting ==================
    plot_strong_scalability_by_payload(
        distributed_df=distributed_df,
        size_filter=10000000,    # 10M
        fixed_threads=16,
        label='Distributed Strong Scalability (Size: 10M, Threads: 16)',
        log_x=True,
        save_fig=True,
        fig_name='distributed_strong_scalability10M.png'
    )
    
    plot_strong_scalability_by_payload(
        distributed_df=distributed_df,
        size_filter=100000000,    # 100M
        fixed_threads=16,
        label='Distributed Strong Scalability (Size: 100M, Threads: 16)',
        log_x=True,
        save_fig=True,
        fig_name='distributed_strong_scalability100M.png'
    )
    
    plot_weak_scalability_by_payload(
        distributed_df=distributed_df,
        base_size=1000000,  # 1M
        fixed_threads=16,
        label='Distributed Weak Scalability (Base Size: 1M, Threads: 16)',
        log_x=True,
        save_fig=True,
        fig_name='distributed_weak_scalability.png'
    )
