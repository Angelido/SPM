#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <hpc_helpers.hpp>

using ull=unsigned long long;

// Function to calculate the number of steps in the Collatz sequence
ull collatz(ull n) {
    ull steps=0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        steps++;
    }

    return steps;
}

int main(int argc, char* argv[]) {

    std::vector<std::pair<ull, ull>> ranges;

    // Check if there are enough arguments
    // argv[0] is the program name, so we start from argv[1]
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " start1-end1 start2-end2 ..." << std::endl;
        return 1;
    }

    // Read the ranges from command line arguments
    // Each argument should be in the format start-end
    for (int i = 1; i < argc; ++i) {
        std::string input(argv[i]);
        size_t dash_pos = input.find('-');

        if (dash_pos == std::string::npos) {
            std::cerr << "Error: Invalid range format '" << input << "' (expected start-end)" << std::endl;
            return 1;
        }

        // Extract start and end from the input string
        ull start = std::stoull(input.substr(0, dash_pos));
        ull end = std::stoull(input.substr(dash_pos + 1));

        // Check if the numbers are valid
        if (start == 0 || end == 0) {
            std::cerr << "Error: Numbers must be positive integers" << std::endl;
            return 1;
        }
        if (start > end) {
            std::cerr << "Error: Start number must be less than or equal to end number" << std::endl;
            return 1;
        }

        ranges.emplace_back(start, end);
    }

    // Print the ranges
    for (const auto& range : ranges) {
        std::cout << "Range: " << range.first << "-" << range.second << std::endl;
    }

    std::vector<ull> maximum(ranges.size(), 0);

    // Start the timer
    TIMERSTART(sequential_collatz);

    int j = 0;
    ull start, end;
    for (const auto& range : ranges) {
        start = range.first;
        end = range.second;

        for (ull i = start; i <= end; ++i) {
            // Calculate the maximum steps for the current range
            maximum[j] = std::max(maximum[j], collatz(i));
        }
        ++j;
    }

    TIMERSTOP(sequential_collatz);

    // Print the maximum steps for each range
    for (size_t j = 0; j < ranges.size(); ++j) {
        std::cout << "Range " << ranges[j].first << "-" << ranges[j].second << ": Max steps = " << maximum[j] << std::endl;
    }

    return 0;
}