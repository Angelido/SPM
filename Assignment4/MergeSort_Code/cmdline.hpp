#if !defined(_CMDLINE_HPP)
#define _CMDLINE_HPP

//#include <cstdio>
//#include <iostream>
//#include <cstring>
//#include <string>
#include <unistd.h> //getopt

#include <config.hpp>
#include <general_helpers.hpp>

static inline void usage(const char* argv0) {
    std::printf("--------------------------\n");
    std::printf("Usage: %s [options]\n", argv0);
    std::printf("\nOptions:\n");
    std::printf(" -s N: array size (defaults to %lld).\n", SIZE);
    std::printf(" -r R: record payload, in bytes (defaults to %d).\n", RPAYLOAD);
    if (!std::strstr(argv0, "seq")) {
        std::printf(" -t T: number of FastFlow threads(defaults to %d).\n", N_THREADS);
    }
    std::printf("--------------------------\n");
}

// Assign to the global variables the values passed by the command line
int parseCommandLine(int argc, char* argv[]) {
    extern char* optarg;
    const std::string optstr = (std::strstr(argv[0], "seq") != nullptr) ? "s:r:" : "s:r:t:";    
    long opt = 1;

    while ((opt = getopt(argc, argv, optstr.c_str())) != -1) {
        switch (opt) {
            case 'r': {
                long R = 0;
                if (!optarg || !isNumber(optarg, R)) {
                    std::fprintf(stderr, "Error: wrong '-r' option\n");
                    usage(argv[0]);
                    return -1;
                }
                RPAYLOAD = R;
            } break;
            case 's': {
                long long N = 0;
                if (!optarg || !parseLen(optarg, N)) {
                   std::fprintf(stderr, "Error: wrong '-s' option\n");
                   usage(argv[0]);
                }
                SIZE = N;
            } break;
            case 't': {
                long T = 0; 
                if (!optarg || !isNumber(optarg, T) || T < 1) {
                   std::fprintf(stderr, "Error: wrong '-t' option\n");
                   usage(argv[0]);
                   return -1;
                }
                N_THREADS = T;
            } break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    return 0;
}



#endif // _CMDLINE_HPP