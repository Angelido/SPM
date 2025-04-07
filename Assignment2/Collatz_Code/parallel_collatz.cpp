#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <hpc_helpers.hpp>

using ull = unsigned long long;

// Global variables
int num_threads = 16;
int chunk_size = 1;
bool dynamic = false;

// Struct for storing Collatz data
struct CollatzData {
    std::vector<std::pair<ull, ull>> ranges;
    std::vector<ull> max_steps_per_range;
    std::mutex max_mutex;
    std::vector<std::unique_ptr<struct DynamicTaskManager>> task_managers; // For dynamic task management
};

// Struct for managing dynamic tasks
struct DynamicTaskManager {
    DynamicTaskManager(ull start, ull end, int chunk_size) 
        : current(start), end(end), chunk_size(chunk_size) {}

    // Function to get the next task
    // Returns false if there are no more tasks
    bool get_next_task(ull& task_start, ull& task_end) {
        ull old = current.fetch_add(chunk_size, std::memory_order_relaxed);
        if (old > end) return false;
        task_start = old;
        task_end = std::min(old + static_cast<ull>(chunk_size) - 1, end);
        return true;
    }

    // Eliminate copy and move constructors and assignment operators
    DynamicTaskManager(const DynamicTaskManager&) = delete;
    DynamicTaskManager& operator=(const DynamicTaskManager&) = delete;
    DynamicTaskManager(DynamicTaskManager&&) = delete;
    DynamicTaskManager& operator=(DynamicTaskManager&&) = delete;

private:
    std::atomic<ull> current;
    ull end;
    int chunk_size;
};

// Function to calculate the number of steps in the Collatz sequence
ull collatz(ull n) {
    ull steps = 0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        steps++;
    }
    return steps;
}

// Function that implements the dynamic policy
void dynamic_policy(CollatzData &data) {
    ull task_start, task_end;
    ull local_max;

    for (size_t j = 0; j < data.ranges.size(); ++j) {
        local_max = 0;
        
        // Ask the task manager for the next task
        while (data.task_managers[j]->get_next_task(task_start, task_end)) {
            for (ull i = task_start; i <= task_end; ++i) {
                local_max = std::max(local_max, collatz(i));
            }
        }

        {
            // Lock the mutex to update the maximum steps for the range
            std::lock_guard<std::mutex> lock(data.max_mutex);
            data.max_steps_per_range[j] = std::max(data.max_steps_per_range[j], local_max);
        }
    }
}

// Function that implements the block-cyclic policy
void block_cyclic_policy(CollatzData &data, int thread_id) {
    ull start, end;
    ull shift = num_threads * chunk_size;
    ull local_max;
    for (size_t j = 0; j < data.ranges.size(); ++j) {
        start = data.ranges[j].first;
        end = data.ranges[j].second;
        local_max = 0;
        // Each thread processes its own chunk of the range
        for (ull i = start + thread_id * chunk_size; i <= end; i += shift) {
            for (int k = 0; k < chunk_size && (i + k) <= end; ++k) {
                local_max = std::max(local_max, collatz(i + k));
            }
        }
        {
            // Lock the mutex to update the maximum steps for the range
            std::lock_guard<std::mutex> lock(data.max_mutex);
            data.max_steps_per_range[j] = std::max(data.max_steps_per_range[j], local_max);
        }
    }
}

// Function to run the Collatz calculation based on the selected policy
void run(CollatzData &data) {
    std::vector<std::thread> threads;

    if (dynamic) {
        TIMERSTART(parallel_collatz_dynamic);

        // Creation of threads for the dynamic policy
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(dynamic_policy, std::ref(data));
        }
        for (auto &t : threads)
            t.join();
        TIMERSTOP(parallel_collatz_dynamic);
    } else {
        TIMERSTART(parallel_collatz_static);

        // Creation of threads for the block-cyclic policy
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(block_cyclic_policy, std::ref(data), i);
        }
        for (auto &t : threads)
            t.join();
        TIMERSTOP(parallel_collatz_static);
    }
}

// Function to check if a string is a number
bool is_number(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

int main(int argc, char* argv[]) {

    // Default values
    dynamic = false;
    num_threads = 16;
    chunk_size = 1;
    std::vector<std::pair<ull, ull>> ranges;

    // Check if there are enough arguments
    // argv[0] is the program name, so we start from argv[1]
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-d] [-n num_threads] [-c chunk_size] start-end [...]" << std::endl;
        return 1;
    }

    // Parsing command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-d") {         //Check for dynamic mode
            dynamic = true;
        } else if (arg == "-n") {  // Number of threads

            // Check if the next argument is a number
            if (i + 1 < argc && is_number(argv[i + 1])) {
                num_threads = std::stoi(argv[++i]);
            } else {  // If not, print an error message
                std::cerr << "Error: Missing or invalid value for -n option" << std::endl;
                return 1;
            }
        } else if (arg == "-c") { // Chunk size
            
            // Check if the next argument is a number
            if (i + 1 < argc && is_number(argv[i + 1])) {
                chunk_size = std::stoi(argv[++i]);
            } else {  // If not, print an error message
                std::cerr << "Error: Missing or invalid value for -c option" << std::endl;
                return 1;
            }
        } else {    // Range input
            size_t dash_pos = arg.find('-');

            // Check if the argument is in the correct format
            if (dash_pos == std::string::npos) {
                std::cerr << "Error: Invalid range format '" << arg << "' (expected start-end)" << std::endl;
                return 1;
            }
            ull start = std::stoull(arg.substr(0, dash_pos));
            ull end = std::stoull(arg.substr(dash_pos + 1));

            // Check if the numbers are valid
            if (start > end) {
                std::cerr << "Error: Start number must be less than or equal to end number" << std::endl;
                return 1;
            }

            ranges.emplace_back(start, end);
        }
    }

    // Initialize the CollatzData structure
    CollatzData data;
    data.ranges = ranges;
    data.max_steps_per_range.resize(ranges.size(), 0);

    // For dynamic mode, create task managers for each range
    for (const auto &range : ranges) {
        data.task_managers.push_back(std::make_unique<DynamicTaskManager>(range.first, range.second, chunk_size));
    }

    // Print the configuration
    std::cout << "Dynamic mode: " << (dynamic ? "ON" : "OFF") << std::endl;
    std::cout << "Number of threads: " << num_threads << std::endl;
    std::cout << "Number of tasks (chunk size): " << chunk_size << std::endl;
    std::cout << "Ranges:" << std::endl;
    for (const auto &range : data.ranges) {
        std::cout << range.first << "-" << range.second << std::endl;
    }

    // Run the Collatz calculation
    run(data);

    // Print the maximum steps for each range
    for (size_t i = 0; i < data.ranges.size(); ++i) {
        std::cout << "Range " << data.ranges[i].first << "-" 
                  << data.ranges[i].second
                  << ": Max steps = " << data.max_steps_per_range[i] << std::endl;
    }

    return 0;
}