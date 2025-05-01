#include <config.hpp>
#include <cmdline.hpp>
#include <utility.hpp>
#include <hpc_helpers.hpp>
#include <omp.h>
#include <vector>
#include <filesystem>

int main(int argc, char *argv[]) {
    // Ensure at least one argument (file or directory) is provided
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    // Parse command-line options; `start` is index of first path argument
    long start=parseCommandLine(argc, argv);
    if (start<0) return -1;
  
    // Flatten: collect all target files into a single list
    std::vector<std::string> fileList;

    // ----------------- start timer -----------------
    TIMERSTART(parallel_miniz);

    for (int i = (int)start; i < argc; ++i) {
        std::filesystem::path p = argv[i];
        
        // If argument is a directory, iterate its entries
        if (std::filesystem::is_directory(p)) {

            // Choose recursive or single-level iteration
            if (RECUR) { // Recursive iteration
                for (auto &ent : std::filesystem::recursive_directory_iterator(p)) {
                    if (!ent.is_regular_file()) continue;
                    const char* fname = ent.path().c_str();

                    // Check if the file should be discarded based on its suffix
                    bool discard = discardIt(fname, COMP);
                    if (discard) {
                        // Verbose mode: report ignored files
                        if (QUITE_MODE >= 2) {
                            if (COMP)
                                std::fprintf(stderr, "%s has already a %s suffix -- ignored\n", fname, SUFFIX);
                            else
                                std::fprintf(stderr, "%s does not have a %s suffix -- ignored\n", fname, SUFFIX);
                        }
                    } else {
                        // Add valid file to the list
                        fileList.push_back(ent.path().string());
                    }
                }
            } else { // Only look at the immediate children of the directory
                for (auto &ent : std::filesystem::directory_iterator(p)) {
                    if (!ent.is_regular_file()) continue;
                    const char* fname = ent.path().c_str();
                    bool discard = discardIt(fname, COMP);
                    if (discard) {
                        // Verbose mode: report ignored files
                        if (QUITE_MODE >= 2) {
                            if (COMP)
                                std::fprintf(stderr, "%s has already a %s suffix -- ignored\n", fname, SUFFIX);
                            else
                                std::fprintf(stderr, "%s does not have a %s suffix -- ignored\n", fname, SUFFIX);
                        }
                    } else {
                        // Add valid file to the list
                        fileList.push_back(ent.path().string());
                    }
                }
            }
        } else { // Single file argument
            const char* fname = p.c_str();
            bool discard = discardIt(fname, COMP);
            if (discard) {
                // Verbose mode: report ignored files
                if (QUITE_MODE >= 2) {
                    if (COMP)
                        std::fprintf(stderr, "%s has already a %s suffix -- ignored\n", fname, SUFFIX);
                    else
                        std::fprintf(stderr, "%s does not have a %s suffix -- ignored\n", fname, SUFFIX);
                }
            } else {
                // Add valid file to the list
                fileList.push_back(p.string());
            }
        }
    }
    
    // Check if any files were found
    int nFiles = (int)fileList.size();
    if (nFiles == 0) {
        std::fprintf(stderr, "No files to process\n");
        return -1;
    }

    // OpenMP tuning: nested ON, 2 levels
    int num_threads = 32;
    omp_set_dynamic(0);            // Disable dynamic teams
    omp_set_nested(1);             // Enable nested parallelism
    omp_set_max_active_levels(2);  // Allow two nested levels: outer + inner
    omp_set_num_threads(num_threads);

    // Dispatch tasks: one task per file
    bool success = true;
    #pragma omp parallel
    {
        #pragma omp single
        {
            for (const auto &f : fileList) {
                #pragma omp task firstprivate(f) shared(success)
                {
                    bool r;

                    if(COMP) {
                        // Parallel chunked compression
                        r = compressParallelFile(f.c_str());
                    } 
                    else {
                        // Parallel chunked decompression
                        r = decompressParallelFile(f.c_str());
                    }
                    // Combine results atomically
                    #pragma omp atomic
                    success &= r;
                }
            }
            #pragma omp taskwait
        }
    }
    TIMERSTOP(parallel_miniz);
    // ------------------- end timer -----------------
    
    // Final status
	if (!success) {
		printf("Exiting with (some) Error(s)\n");
		return -1;
	}
	printf("Exiting with Success\n");
	return 0;
}