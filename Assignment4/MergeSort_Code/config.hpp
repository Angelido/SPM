#if !defined(_CONFIG_HPP)
#define _CONFIG_HPP

#include <iostream>
#include <cstring>  // per std::memcpy, std::memset

using ull = unsigned long long;

// Threshold for the base case of the merge sort algorithm
static const size_t BASE_CASE_SIZE = 1000;

// Global variables with their default values
static int RPAYLOAD  = 32;               // record payload (n° of bytes)
static int N_THREADS = 8;                // n° of fastflow threads
static ull SIZE      = 1'000'000;        // array length

struct Record {
    unsigned long key;
    char* payload;

    // Default constructor
    Record()
      : key(0), payload(new char[RPAYLOAD]) {
        std::memset(payload, 0, RPAYLOAD);
    }

    // Copy constructor (deep copy)
    Record(const Record& other)
      : key(other.key), payload(new char[RPAYLOAD]) {
        std::memcpy(payload, other.payload, RPAYLOAD);
    }

    // Copy assignment operator (deep copy)
    Record& operator=(const Record& other) {
        if (this != &other) {
            key = other.key;

            // Only allocate new payload if necessary
            if (!payload) {
                payload = new char[RPAYLOAD];
            }

            std::memcpy(payload, other.payload, RPAYLOAD);
        }
        return *this;
    }

    // Move constructor
    Record(Record&& other) noexcept
      : key(other.key), payload(other.payload) {
        other.payload = nullptr;
    }

    // Move assignment operator
    Record& operator=(Record&& other) noexcept {
        if (this != &other) {
            delete[] payload;
            key = other.key;
            payload = other.payload;
            other.payload = nullptr;
        }
        return *this;
    }

    // Distructore
    ~Record() {
        delete[] payload;
    }
};

#endif // _CONFIG_HPP
