#if !defined(_GNRL_HLP_HPP)
#define _GNRL_HLP_HPP

//#include <string>
#include <cstring>
//#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <random>


// check if the string 's' is a number, otherwise it returns false
template<typename L>
static bool isNumber(const char* s, L &n) {
    try {
		/*e will store the position of the first
		  character in s that is not part of the number*/
		size_t e;
		n = std::stol(s, &e, 10);
		return e == strlen(s);
    } catch (const std::invalid_argument&) {
		return false;
    } catch (const std::out_of_range&) {
		return false;
    }
}

static bool parseLen(const char* in, long long &N) {
    // Check for suffix 'K', 'M', 'G', 'B' (case insensitive)
    size_t len = std::strlen(in);
    if (len > 1) {
        char suffix = in[len - 1];
        long long multiplier = 1;
        if (suffix == 'K' || suffix == 'k') {
            multiplier = 1000LL;
        } else if (suffix == 'M' || suffix == 'm') {
            multiplier = 1000000LL;
        } else if (suffix == 'G' || suffix == 'g' || suffix == 'B' || suffix == 'b') {
            multiplier = 1000000000LL;
        }
        if (multiplier != 1) {
            std::string numPart(in, len - 1);
            long long base = 0;
            if (!isNumber(numPart.c_str(), base)) {
                /*std::fprintf(stderr, "Error: wrong '-s' option\n");
                  usage(argv[0]);*/
                return false;
            }
            N = base * multiplier;
            return true;
        }
    }
    // No suffix, just parse as number
    if (!isNumber(in, N)) {
        /*std::fprintf(stderr, "Error: wrong '-s' option\n");
          usage(argv[0]);*/
        return false;
    }
    return true;
}


void rand_init_ints(std::vector<Record>& v, int seed = 69420) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<unsigned long> dist(0, v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        v[i].key = dist(gen);
    }
}


bool is_sorted_keys(const std::vector<Record>& v) {
    for (size_t i = 1; i < v.size(); ++i) {
        if (v[i - 1].key > v[i].key)
            return false;
    }
    return true;
}

#endif // _GNRL_HLP_HPP