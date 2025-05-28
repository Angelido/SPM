
#include <config.hpp>
#include <cmdline.hpp>
#include <hpc_helpers.hpp>

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <typeinfo>
#include <cxxabi.h>  

// Merge function that merges two sorted subarrays
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

// Sequential mergesort function
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



int main(int argc, char* argv[]) {
    bool asks_help = argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h");
    if (asks_help) {
        usage(argv[0]);
        return -1;
    }

    // Parse command line arguments and set some global variables
    long status = 0;
    if (argc > 1) 
        status = parseCommandLine(argc, argv);
    if (status < 0) return -1;

    std::cout << "Record payload: " << RPAYLOAD << std::endl;
    std::cout << "Array size: " << SIZE << std::endl;

    std::vector<Record> data(SIZE);
    std::vector<Record> tmp(SIZE);
    // Initialize the array with random integers
    rand_init_ints(data);  


    TIMERSTART(sequential_mergesort);

    mergesort_seq(data, tmp, 0, SIZE); 

    TIMERSTOP(sequential_mergesort);

    if (is_sorted_keys(data)) {
        std::cout << "Array sorted correctly.\n";
    } else {
        std::cout << "Error: array not sorted!\n";
    }

    return 0;
}