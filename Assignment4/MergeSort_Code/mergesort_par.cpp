#include <config.hpp>
#include <cmdline.hpp>
#include <hpc_helpers.hpp>
#include <general_helpers.hpp>

#include <algorithm>
#include <deque>
#include <iostream>
#include <vector>
#include <ff/ff.hpp>

using namespace ff;

// ---------------------------- struct -----------------------------------
struct Task {
    size_t left;
    size_t mid;
    size_t right;
    bool is_sorted;
    int id;
};


// ---------------------------- sequential merge  -------------------
void merge(std::vector<Record>& v, std::vector<Record>& dst,
           size_t left, size_t mid, size_t right) {
    size_t i = left, j = mid, k = left;

    while (i < mid && j < right) {
        if (v[i].key <= v[j].key) {
            dst[k++] = v[i++];
        } else {
            dst[k++] = v[j++];
        }
    }

    while (i < mid) {
        dst[k++] = v[i++];
    }

    while (j < right) {
        dst[k++] = v[j++];
    }

    // Copy result in v
    std::copy(dst.begin() + left, dst.begin() + right, v.begin() + left);
}


// ---------------------------- sequential mergesort -------------------
void mergesort_seq(std::vector<Record>& v, std::vector<Record>& tmp, size_t left, size_t right) {
    size_t size = right - left;

    if (size <= BASE_CASE_SIZE) {
        std::stable_sort(v.begin() + left, v.begin() + right,
                         [](const Record& a, const Record& b) {
                             return a.key < b.key;
                         });
        return;
    }

    size_t mid = left + size / 2;

    mergesort_seq(v, tmp, left, mid);
    mergesort_seq(v, tmp, mid, right);

    merge(v, tmp, left, mid, right);
}


// -------------------------- Master function -------------------
struct Master: ff_monode_t<Task>{
    Master(std::vector<Record> &data, std::vector<Record> &tmp): data(data), tmp(tmp) {}

    // ---------------------------- send task -------------------
    void send_task(bool from_left) {
        std::deque<std::pair<size_t, size_t>> temp_deque;
        int steps = 0;

        if (from_left) {
            // From left to right
            while (!ranges.empty()) {
                if (ranges.size() >= 2) {
                    // Take the first two intervals
                    auto first = ranges.front(); ranges.pop_front();
                    auto second = ranges.front(); ranges.pop_front();

                    // Create a new task
                    Task* task = new Task{first.first, second.first, second.second, true, steps};
                    ff_send_out(task);
                    steps++;

                    // Add the pair (left, right) to the temporary deque
                    temp_deque.push_back({first.first, second.second});
                } else {
                    // Odd case: element without a partner
                    auto leftover = ranges.front(); ranges.pop_front();
                    temp_deque.push_back(leftover);
                }
            }
        } else {
            // From right to left
            while (!ranges.empty()) {
                if (ranges.size() >= 2) {
                    // Take the last two intervals
                    auto second = ranges.back(); ranges.pop_back();
                    auto first = ranges.back(); ranges.pop_back();

                    // Create a new task
                    Task* task = new Task{first.first, second.first, second.second, true, steps};
                    ff_send_out(task);
                    steps++;

                    // Add the pair (left, right) to the temporary deque
                    temp_deque.push_front({first.first, second.second}); 
                } else {
                    // Odd case: element without a partner
                    auto leftover = ranges.back(); ranges.pop_back();
                    temp_deque.push_front(leftover);
                }
            }
        }

        // Update the ranges with the new pairs
        ranges = std::move(temp_deque);

        // Change the direction for the next round
        from_left = !from_left;

        // Save the number of sent tasks
        n_sent = steps;
    }


    Task* svc(Task* t) {

        // If the task is null, it means we are in the first phase
        if(t==nullptr) {
            const int n_workers = get_num_outchannels();

            size_t start=0;
            size_t lenght = data.size();

            // Compute the ranges for each worker
            size_t total_parts = n_workers+1;
            size_t chunk_size = lenght / total_parts;
            int remainder = lenght % total_parts;
            size_t end;

            // Distribute the ranges among the workers
            for (int i = 0; i < n_workers; ++i) {
                end = start + chunk_size + (i < remainder ? 1 : 0);
                ranges.push_back({start, end});
                Task* task = new Task{start, start, end, false, i};
                ff_send_out(task);
                start = end;
            }
            // Handle the last part
            end=lenght;
            ranges.push_back({start, end});

            // Sequentially sort the last part
            mergesort_seq(data, tmp, start, end);

            n_sent = n_workers;
            return GO_ON;
        }

        // If n_sent is 0, it means all the tasks have been received
        if(!--n_sent) {
            
            // If size of ranges is 1, it means we have sorted all parts
            if(ranges.size()==1) {
                broadcast_task(EOS); // Broadcast EOS to all workers
                delete t;
                return GO_ON;
            } else {
                // If there are still ranges to process, send the next task
                send_task(from_left);
                return GO_ON;
            }
        } else {  // If there are still tasks to receive
            delete t;
            return GO_ON;
        }

        return GO_ON;
    }


    std::vector<Record> &data;
    std::vector<Record> &tmp;
    std::deque<std::pair<size_t, size_t>> ranges;
    int n_sent;
    bool from_left = true;
};


// ---------------------------- Worker function -------------------
struct Worker: ff_node_t<Task> {
    Worker(std::vector<Record> &data, std::vector<Record> &tmp): data(data), tmp(tmp) {}

    Task* svc(Task* task) {

        // If the task is not sorted we are in the first phase
        if(!task->is_sorted) {
            size_t left = task->left;
            size_t right = task->right;

            // Sequentially sort the part of the array assigned to this worker
            mergesort_seq(data, tmp, left, right);

            task->is_sorted = true;

            // Return the task to the master
            ff_send_out(task);

            return GO_ON;
        } else { // Else we are in the second phase, and we have to merge
            size_t left = task->left;
            size_t mid = task->mid;
            size_t right = task->right;
            // Merge the two sorted parts
            merge(data, tmp, left, mid, right);

            ff_send_out(task);
            return GO_ON;
        }
    }

    std::vector<Record> &data;
    std::vector<Record> &tmp;
};



// ---------------------------- main -----------------------------------
int main(int argc, char* argv[]) {

    // Help & cmdline
    bool asks_help = argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h");
    if (asks_help) { usage(argv[0]); return -1; }
    long status = 0;
    if (argc > 1) status = parseCommandLine(argc, argv);
    if (status < 0) return -1;

    std::cout << "Record payload: " << RPAYLOAD << std::endl;
    std::cout << "Array size: " << SIZE << std::endl;
    std::cout << "N° cores according to FastFlow: " << N_THREADS << std::endl;

    // Initialize vector of random integers
    std::vector<Record> data(SIZE);
    std::vector<Record> tmp(SIZE);
    rand_init_ints(data); 

    // If N_THREADS is 1, we run the sequential version of mergesort
    if (N_THREADS == 1) {

        TIMERSTART(parallel_sort);
        mergesort_seq(data, tmp, 0, SIZE);
        TIMERSTOP(parallel_sort);

        if (is_sorted_keys(data)) {
            std::cout << "Array sorted correctly.\n";
        } else {
            std::cout << "Error: array not sorted!\n";
        }
        return 0;
    }

    // If N_THREADS > 1, we run the parallel version of mergesort using FastFlow

    ff_farm farm; // Create a farm for parallel processing
    // Use a master node for the emitter
    Master master(data, tmp);
    farm.add_emitter(master);
    // Use N_THREADS-1 workers for parallel processing
    std::vector<ff_node*> workers;
    for (int i = 0; i < N_THREADS-1; ++i) {
        workers.push_back(new Worker(data, tmp));
    }
    farm.add_workers(workers);
    farm.remove_collector(); // Remove the collector node
    farm.wrap_around(); // Wrap around to create a feedback channel
    farm.cleanup_workers(); // Enable worker cleanup

    TIMERSTART(parallel_sort);

    // Run the farm and wait for it to finish
    if (farm.run_and_wait_end()<0) {
        error("running farm");
        return -1;
    }

    TIMERSTOP(parallel_sort);

    // Verification
    if (is_sorted_keys(data)) {
        std::cout << "Array sorted correctly.\n";
    } else {
        std::cout << "Error: array not sorted!\n";
    }  

    return 0;
}