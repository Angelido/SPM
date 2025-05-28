#include <config.hpp>
#include <cmdline.hpp>
#include <hpc_helpers.hpp>
#include <general_helpers.hpp>

#include <algorithm>
#include <deque>
#include <iostream>
#include <vector>
#include <ff/ff.hpp>
#include <mpi.h>

using namespace ff;
using ull = unsigned long long;
size_t CHUNK_SIZE = 5'000'000;  
double t_start, t_end;

// ---------------------------- struct -----------------------------------
struct Task {
    size_t left;
    size_t mid;
    size_t right;
    bool is_sorted;
    int id;
};

// ---------------------------- error checking macro -------------------
#define CHECK_ERROR(err) do {								\
	if (err != MPI_SUCCESS) {								\
		char errstr[MPI_MAX_ERROR_STRING];					\
		int errlen=0;										\
		MPI_Error_string(err,errstr,&errlen);				\
		std::cerr											\
			<< "MPI error code " << err						\
			<< " (" << std::string(errstr,errlen) << ")"	\
			<< " line: " << __LINE__ << "\n";				\
		MPI_Abort(MPI_COMM_WORLD, err);						\
		std::abort();										\
	}														\
} while(0)


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

    // Copy result back to v
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


// -------------------------- FasfFlow Master function -------------------
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

        // Change the direction for the next call
        from_left = !from_left;

        // Save the number of sent tasks
        n_sent = steps;
    }


    Task* svc(Task* t) {
        // If the task is null, it means we are in the initial phase
        if(t==nullptr) {
            const int n_workers = get_num_outchannels();

            size_t start=0;
            size_t lenght = data.size();

            // Divide the data into chunks for each worker
            size_t total_parts = n_workers+1;
            size_t chunk_size = lenght / total_parts;
            int remainder = lenght % total_parts;
            size_t end;

            // Send initial tasks to workers
            for (int i = 0; i < n_workers; ++i) {
                end = start + chunk_size + (i < remainder ? 1 : 0);
                ranges.push_back({start, end});
                Task* task = new Task{start, start, end, false, i};
                ff_send_out(task);
                start = end;
            }
            // Handle the last chunk
            end=lenght;
            ranges.push_back({start, end});

            // Sequentially sort the last chunk
            mergesort_seq(data, tmp, start, end);

            n_sent = n_workers;
            return GO_ON;
        }

        // If we have receveid all the tasks, we can merge
        if(!--n_sent) {
            
            // If ranges is only one element, we have finished sorting
            if(ranges.size()==1) {
                broadcast_task(EOS);
                delete t;
                return GO_ON;
            } else {
                send_task(from_left);
                return GO_ON;
            }

        } else {
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

        // If is_sorted is false, we are in the initial phase
        if(!task->is_sorted) {
            size_t left = task->left;
            size_t right = task->right;

            // Sequentially sort the part of the array assigned to this worker
            mergesort_seq(data, tmp, left, right);

            // Set is_sorted to true to indicate that this part is sorted
            task->is_sorted = true;

            ff_send_out(task);

            return GO_ON;
        } else { // We are in the merging phase
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

// ---------------------------- internal work function -------------------
// This function is used to run the mergesort algorithm in parallel 
// using FastFlow on each node
void internal_work(std::vector<Record>& data, std::vector<Record>& tmp) {
    
    // If N_THREADS is 1, we run the sequential version of mergesort
    if (N_THREADS == 1) {

        mergesort_seq(data, tmp, 0, SIZE);
        return;
    }

    // If N_THREADS > 1, we run the parallel version of mergesort using FastFlow
    ff_farm farm;
    Master master(data, tmp);
    farm.add_emitter(master);
    std::vector<ff_node*> workers;
    for (int i = 0; i < N_THREADS-1; ++i) {
        workers.push_back(new Worker(data, tmp));
    }

    farm.add_workers(workers);
    farm.remove_collector();  // Remove the collector node 
    farm.wrap_around();    // Enable feedback loop for the farm
    farm.cleanup_workers(); // Enable worker cleanup after processing

    if (farm.run_and_wait_end()<0) {
        error("running farm");
        return;
    }
}

// ---------------------------- get merge partners -------------------
// This function determines the merge partners for each process in the MPI world
std::vector<std::pair<int, bool>> get_merge_partners(int rank, int size) {
    std::vector<std::pair<int, bool>> partners;

    for (int step = 1; step < size; step *= 2) {
        int group_size = 2 * step;

        if (rank % group_size == 0) {
            int partner = rank + step;
            if (partner < size)
                // This process receives from partner
                partners.emplace_back(partner, false); 
        } else if (rank % group_size == step) {
            int partner = rank - step;
            // This process sends to partner
            partners.emplace_back(partner, true); 
            // Once sent, it does not participate further
            break; 
        }
    }
    return partners;
}


// ---------------------------- MPI Master function -------------------
void MPI_Master(std::vector<Record>& data, std::vector<Record>& tmp, int size) {

    int local_n = SIZE / size;
    int remainder = SIZE % size;

    // Compute the number of records for each process
    std::vector<int> counts(size); // Counts of records for each process
    std::vector<int> displs(size); // Displacements for each process
    int offset = 0;
    for (int i = 0; i < size; ++i) {
        counts[i] = local_n + (i < remainder ? 1 : 0);
        displs[i] = offset;
        offset += counts[i];
    }

    // Send the total number of records to each node
    for (int rank = 1; rank < size; ++rank) {
        int total = counts[rank];
        int start = displs[rank];

        // 1) Send the total number of records to each node
        int error = MPI_Send(&total, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
        CHECK_ERROR(error);

        // 2) Send the data in chunks to avoid memory issues
        for (int i = 0; i < total; i += CHUNK_SIZE) {
            int chunk_size = std::min(static_cast<int>(CHUNK_SIZE), total - i);

            std::vector<unsigned long long> keys(chunk_size);
            std::vector<char> payloads(chunk_size * RPAYLOAD);

            // Save the keys and payloads for this chunk
            for (int j = 0; j < chunk_size; ++j) {
                keys[j] = data[start+i+j].key;
                std::memcpy(&payloads[j * RPAYLOAD], data[start+i+j].payload, RPAYLOAD);
            }

            // Send the chunk size
            error = MPI_Send(&chunk_size, 1, MPI_INT, rank, 1, MPI_COMM_WORLD);
            CHECK_ERROR(error);
            // Send keys
            error = MPI_Send(keys.data(), chunk_size, MPI_UNSIGNED_LONG_LONG, rank, 2, MPI_COMM_WORLD);
            CHECK_ERROR(error);
            // Send payloads
            error = MPI_Send(payloads.data(), chunk_size * RPAYLOAD, MPI_BYTE, rank, 3, MPI_COMM_WORLD);
            CHECK_ERROR(error);
        }
    }

    int my_nrecords = counts[0];
    std::vector<Record> my_data;  


    // Initialize the local data for the master process
    my_data = std::vector<Record>(data.begin(), data.begin() + my_nrecords);
    data.clear();
    data.shrink_to_fit();

    // Merge sort the local data using FastFlow
    internal_work(my_data, tmp);

    //------ End first phase: now we start the merge phase ------

    // Obtain merge partners for the master process
    std::vector<std::pair<int, bool>> partners = get_merge_partners(0, size);

    MPI_Status status;

    // For each partner, we will receive data (we are in the master)
    for (auto& partner : partners) {
        int pr = partner.first;
       
        // Receive the number of records from the partner
        int nrec;
        int error = MPI_Recv(&nrec, 1, MPI_INT, pr, 0, MPI_COMM_WORLD, &status);
        CHECK_ERROR(error);
        
        // Receive the data in chunks
        int receveid = 0;
        while(receveid < nrec) {
            int chunk_size;
            // Receive chunk size (tag 1)
            error = MPI_Recv(&chunk_size, 1, MPI_INT, pr, 1, MPI_COMM_WORLD, &status);
            CHECK_ERROR(error);

            std::vector<unsigned long long> keys(chunk_size);
            std::vector<char> payloads(chunk_size * RPAYLOAD);

            // Receive keys (tag 2)
            error = MPI_Recv(keys.data(), chunk_size,
                             MPI_UNSIGNED_LONG_LONG, pr, 2, MPI_COMM_WORLD, &status);
            CHECK_ERROR(error);
            // Receive payloads (tag 3)
            error = MPI_Recv(payloads.data(), chunk_size * RPAYLOAD,
                             MPI_BYTE, pr, 3, MPI_COMM_WORLD, &status);
            CHECK_ERROR(error);

            // Reconstruct the records and add them to my_data
            size_t old_size = my_data.size();
            for (int i = 0; i < chunk_size; ++i) {
                Record r;
                r.key = keys[i];
                std::memcpy(r.payload, &payloads[i * RPAYLOAD], RPAYLOAD);
                my_data.push_back(std::move(r));
            }

            // Merge in the sequantial way
            merge(my_data, tmp, 0, old_size, my_data.size());

            receveid += chunk_size;
        }
        
    }

    // At this point, my_data contains the final sorted data
    // Stop the timer
    t_end = MPI_Wtime();
    std::cout << "Total time (without init): " << (t_end - t_start) << " s\n";

    if (is_sorted_keys(my_data)) std::cout << "Array sorted correctly.\n";
    else std::cout << "Error: array not sorted!\n";
}


// ---------------------------- MPI Worker function -------------------
void MPI_Worker(int rank, int size) {
    const int master = 0;
    MPI_Status status;
    int error;

    // Receive the total number of records from the master
    int total_n;
    error = MPI_Recv(&total_n, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    CHECK_ERROR(error);

    // Pre-allocate memory for the local data
    std::vector<Record> local_data;
    local_data.reserve(total_n);

    // Receive the data in chunks
    int received = 0;
    while (received < total_n) {
        int chunk_size;
        // Receive chunk size (tag 1)
        error = MPI_Recv(&chunk_size, 1, MPI_INT, master, 1, MPI_COMM_WORLD, &status);
        CHECK_ERROR(error);

        // Temporary vectors for this chunk
        std::vector<unsigned long long> keys(chunk_size);
        std::vector<char> payloads(chunk_size * RPAYLOAD);

        // Receive keys (tag 2)
        error = MPI_Recv(keys.data(), chunk_size,
                         MPI_UNSIGNED_LONG_LONG, master, 2, MPI_COMM_WORLD, &status);
        CHECK_ERROR(error);
        // Receive payloads (tag 3)
        error = MPI_Recv(payloads.data(), chunk_size * RPAYLOAD,
                         MPI_BYTE, master, 3, MPI_COMM_WORLD, &status);
        CHECK_ERROR(error);

        // Reconstruct the records and add them to local_data
        for (int i = 0; i < chunk_size; ++i) {
            Record r;
            r.key = keys[i];
            std::memcpy(r.payload, &payloads[i * RPAYLOAD], RPAYLOAD);
            local_data.push_back(std::move(r));
        }
        received += chunk_size;
    }

    // Local data is now filled with the records received from the master
    std::vector<Record> local_tmp(local_data.size());

    // Sort the local data using FastFlow
    internal_work(local_data, local_tmp);

    //-- End first phase: now we start the merge phase ---

    // Compute merge partners for this worker
    std::vector<std::pair<int, bool>> partners = get_merge_partners(rank, size);

    // For each partner, we will either send or receive data
    for (const auto& partner : partners) {
        int pr = partner.first;
        bool is_sender = partner.second;

        if (!is_sender) {
            // Receiver: receive the sorted data from the partner
            int nrec;
            error = MPI_Recv(&nrec, 1, MPI_INT, pr, 0, MPI_COMM_WORLD, &status);
            CHECK_ERROR(error);
            
            int receveid = 0;
            while(receveid < nrec) {
                int chunk_size;

                error = MPI_Recv(&chunk_size, 1, MPI_INT, pr, 1, MPI_COMM_WORLD, &status);
     
                std::vector<unsigned long long> keys(chunk_size);
                std::vector<char> payloads(chunk_size * RPAYLOAD);

                error = MPI_Recv(keys.data(), chunk_size,
                                 MPI_UNSIGNED_LONG_LONG, pr, 2, MPI_COMM_WORLD, &status);
                CHECK_ERROR(error);
                error = MPI_Recv(payloads.data(), chunk_size * RPAYLOAD,
                                 MPI_BYTE, pr, 3, MPI_COMM_WORLD, &status);
                CHECK_ERROR(error);

                size_t old_size = local_data.size();
                for (int i = 0; i < chunk_size; ++i) {
                    Record r;
                    r.key = keys[i];
                    std::memcpy(r.payload, &payloads[i * RPAYLOAD], RPAYLOAD);
                    local_data.push_back(std::move(r));
                }
                local_tmp.resize(local_data.size());
                merge(local_data, local_tmp, 0, old_size, local_data.size());

                receveid += chunk_size;
            }
        } else {
            // Sender: send the sorted data to the partner
            int nrec = static_cast<int>(local_data.size());

            int error = MPI_Send(&nrec, 1, MPI_INT, pr, 0, MPI_COMM_WORLD);
            CHECK_ERROR(error);

            int number_sent = 0;
            while(number_sent < nrec) {

                int chunk_size = std::min(static_cast<int>(CHUNK_SIZE), nrec - number_sent);
                std::vector<unsigned long long> keys(chunk_size);
                std::vector<char> payloads(chunk_size * RPAYLOAD);

                for (int j = 0; j < chunk_size; ++j) {
                    keys[j] = local_data[j].key;
                    std::memcpy(&payloads[j * RPAYLOAD], local_data[j].payload, RPAYLOAD);
                }

                error = MPI_Send(&chunk_size, 1, MPI_INT, pr, 1, MPI_COMM_WORLD);
                CHECK_ERROR(error);
                error = MPI_Send(keys.data(), chunk_size, MPI_UNSIGNED_LONG_LONG, pr, 2, MPI_COMM_WORLD);
                CHECK_ERROR(error);
                error = MPI_Send(payloads.data(), chunk_size * RPAYLOAD, MPI_BYTE, pr, 3, MPI_COMM_WORLD);
                CHECK_ERROR(error);

                number_sent += chunk_size;
            }
        }
    }
}



// ---------------------------- main -----------------------------------
int main(int argc, char* argv[]) {

    // Help & cmdline
    bool asks_help = argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h");
    if (asks_help) { usage(argv[0]); return -1; }
    long status = 0;
    if (argc > 1) status = parseCommandLine(argc, argv);
    if (status < 0) return -1;

    // Initialize MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // If the number of processes is 1, we run the parallel single node version
    if (size==1) {

        std::cout << "Record payload: " << RPAYLOAD << std::endl;
        std::cout << "Array size: " << SIZE << std::endl;
        std::cout << "N° cores according to FastFlow: " << N_THREADS << std::endl;

        std::vector<Record> data(SIZE);
        std::vector<Record> tmp(SIZE);
        rand_init_ints(data); 

        TIMERSTART(parallel_sort);
        internal_work(data, tmp); // Run the parallel mergesort
        TIMERSTOP(parallel_sort);
        

        if (is_sorted_keys(data)) {
            std::cout << "Array sorted correctly.\n";
        } else {
            std::cout << "Error: array not sorted!\n";
        }
    } else { // If the number of processes is greater than 1, we run the MPI version

        if (rank == 0) { // Master process

            if(SIZE < 50'000'001 && RPAYLOAD < 129) {
                CHUNK_SIZE = 10'000'000; 
            }

            if(RPAYLOAD < 129) {
                CHUNK_SIZE = 10'000'000; 
            }

            std::cout << "Record payload: " << RPAYLOAD << std::endl;
            std::cout << "Array size: " << SIZE << std::endl;
            std::cout << "N° cores according to FastFlow: " << N_THREADS << std::endl;
            std::cout << "MPI numbero of nodes: " << size << std::endl;
            std::cout << "Chunk size: " << CHUNK_SIZE << std::endl;

            // Initialize the data and temporary vectors
            std::vector<Record> data(SIZE);
            std::vector<Record> tmp(SIZE);
            rand_init_ints(data); 

            // Start the timer
            t_start = MPI_Wtime();

            // Run the MPI master function that take the data
            MPI_Master(data, tmp, size); 
           
        } else { // Worker processes
            MPI_Worker(rank, size);
        }
    }

    // Finalize MPI
    MPI_Finalize();

    std::cout << "MPI Finalized" << std::endl;
    return 0;
}