#if !defined _UTILITY_HPP
#define _UTILITY_HPP

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include<config.hpp>

// Added by me
#include <vector>   
#include <numeric>   
#include <unistd.h>    // for pwrite
#include <fcntl.h>     // for open
#include <sys/stat.h> // for ftruncate
#include <omp.h>  


// ==================== MY DECLARATIONS ===================

// OLD VERSION, NOT USED 
static inline bool compressDataChunked(unsigned char* ptr, size_t size, const std::string& fname);
static inline bool decompressDataChunked(unsigned char* ptr, size_t size, const std::string& fname);
static inline bool doParallelWork(const char fname[], size_t size, const bool comp);

// FINAL VERSION
static inline bool compressParallelFile(const char *fname);
static inline bool decompressParallelFile(const char *fname);

// ==================== END MY DECLARATIONS ===================


// map the file pointed by filepath in memory
// if size is zero, it looks for file size
// if everything is ok, it returns the memory pointer ptr
static inline bool mapFile(const char fname[], size_t &size, unsigned char *&ptr) {
    // open input file.
    int fd = open(fname,O_RDONLY);
    if (fd<0) {
		if (QUITE_MODE>=1) {
			perror("mapFile open");
			std::fprintf(stderr, "Failed opening file %s\n", fname);
		}
		return false;
    }
    if (size==0) {
		struct stat s;
		if (fstat (fd, &s)) {
			if (QUITE_MODE>=1) {
				perror("fstat");
				std::fprintf(stderr, "Failed to stat file %s\n", fname);
			}
			return false;
		}
		size=s.st_size;
    }
	
    // map all the file in memory
    ptr = (unsigned char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
		if (QUITE_MODE>=1) {
			perror("mmap");
			std::fprintf(stderr, "Failed to memory map file %s\n", fname);
		}
		return false;
    }
    close(fd);
    return true;
}
// unmap a previously memory-mapped file
static inline void unmapFile(unsigned char *ptr, size_t size) {
    if (munmap(ptr, size)<0) {
		if (QUITE_MODE>=1) {
			perror("nummap");
			std::fprintf(stderr, "Failed to unmap file\n");
		}
    }
}
// create an empty file of size 'size' and maps it in memory returning
// the pointer into 'ptr'
static inline bool allocateFile(const char fname[], const size_t size, unsigned char *&ptr) {
	
    int fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "Failed to open output file: %s\n", fname);
        return false;
    }	
    // Resize the file
    if (ftruncate(fd, size) < 0) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "Error setting file size with ftruncate %s\n", strerror(errno));
        close(fd);
        return false;
    }
	
    ptr = (unsigned char*)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "Error mapping file %s\n", strerror(errno));		
        close(fd);
        return false;
    }
	close(fd);
	return true;
}

// check if dir is '.' or '..'
static inline bool isdot(const char dir[]) {
  int l = strlen(dir);  
  if ( (l>0 && dir[l-1] == '.') ) return true;
  return false;
}
// returns true if 'p' is a directory, false if it is a file
// if it is a regular file, filesize will containe the size of the regular file
static inline bool isDirectory(const std::filesystem::path& p, size_t& filesize) {
    try {
		if (std::filesystem::is_directory(p)) {
			return true;
		}
		if (std::filesystem::is_regular_file(p)) {
			filesize = std::filesystem::file_size(p);
			return false;
		}
		filesize=0;
		return false;
    } catch (const std::filesystem::filesystem_error& e) {
		std::fprintf(stderr, "Filesystem error: %s\n", e.what());
        return false;
    }
}

// If compdecomp is true (compressing), it checks if fname has the suffix SUFFIX,
// if yes it returns true
// If compdecomp is false (decompressing), it checks if fname has the suffix SUFFIX,
// if yes it returns false
static inline bool discardIt(const char *fname, const bool compdecomp) {
    const int lensuffix=strlen(SUFFIX);
    const int len      = strlen(fname);
    if (len>lensuffix &&
		(strncmp(&fname[len-lensuffix], SUFFIX, lensuffix)==0)) {
		return compdecomp; // true or false depends on we are compressing or decompressing;
    }
    return !compdecomp;
}

// check if the string 's' is a number, otherwise it returns false
static bool isNumber(const char* s, long &n) {
    try {
		size_t e;
		n=std::stol(s, &e, 10);
		return e == strlen(s);
    } catch (const std::invalid_argument&) {
		return false;
    } catch (const std::out_of_range&) {
		return false;
    }
}
static inline char* getOption(char **begin, char **end, const std::string &option) {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return nullptr;
}

// it compresses the data pointed by 'ptr' and having size 'size'
// fname stores the file name of the file containing the data pointed by 'ptr'
// return true if okay, false in case of errors
static inline bool compressData(unsigned char *ptr, size_t size, const std::string &fname) {
	unsigned char * inPtr  = ptr;
	size_t          inSize = size;
	// get an estimation of the maximum compression size
	size_t cmp_len = compressBound(inSize);
	// allocate memory to store compressed data in memory
	unsigned char *ptrOut = new unsigned char[cmp_len];
	if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "Failed to compress file in memory\n");    
		delete [] ptrOut;
		return false;
	}
	std::string outfile = fname + SUFFIX;
	std::ofstream outFile(outfile, std::ios::binary);
	if (!outFile.is_open()) {
		std::fprintf(stderr, "Failed to open output file: %s\n", outfile.c_str());
		delete [] ptrOut;
		return false;
	}
	
	// Write first the original file size (not really needed, but simplify decompression)
	outFile.write(reinterpret_cast<const char*>(&inSize), sizeof(inSize));	
	// Write the compressed data
	outFile.write(reinterpret_cast<const char*>(ptrOut), cmp_len);
	
	if (REMOVE_ORIGIN) {
		unlink(fname.c_str());
	}
	
	delete[] ptrOut;
	outFile.close();
    return true;
}
// it decompresses the data pointed by 'ptr' and having size 'size'
// fname stores the file name of the file containing the data pointed by 'ptr'
// return true if okay, false in case of errors
static inline bool decompressData(unsigned char *ptr, size_t size, const std::string &fname) {
    size_t decompressedSize= reinterpret_cast<size_t*>(ptr)[0];  // read the original size
	ptr += sizeof(size_t); // advance the pointer

    // Write the decompressed data to a file
	unsigned char *decompressed_data=nullptr;
    std::string outfile = fname.substr(0, fname.size() - 4);  // remove the SUFFIX (i.e., .zip)

	// allocate the space in a file for the uncompressed data. The file is memory mapped.
	if (!allocateFile(outfile.c_str(), decompressedSize, decompressed_data)) return false;
	// decompress the data 	
    if (uncompress(decompressed_data, &decompressedSize, ptr, size) != Z_OK) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "uncompress failed!\n");
		unmapFile(decompressed_data, decompressedSize);
        return false;
    }
	// write the data into the disk
    unmapFile(decompressed_data, decompressedSize);
	
    if (REMOVE_ORIGIN) {
        unlink(fname.c_str());
    }

    return true;
}


// entry-point for compression and decompression 
// returns false in case of error
static inline bool doWork(const char fname[], size_t size, const bool comp) {
    unsigned char *ptr = nullptr;
    if (!mapFile(fname, size, ptr)) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "mapFile %s failed\n", fname);
		return false;
	}
	bool r = (comp) ?
		compressData(ptr, size, fname) :
		decompressData(ptr, size, fname);

    unmapFile(ptr, size);
	return r;
}

// 'dname' is a directory; traverse it and call doWork() for each file
// returns false in case of error
static inline bool walkDir(const char dname[], const bool comp) {
	if (chdir(dname) == -1) {
		if (QUITE_MODE>=1) {
			perror("chdir");
			std::fprintf(stderr, "Error: chdir %s\n", dname);
		}
		return false;
    }
    DIR *dir;	
    if ((dir=opendir(".")) == NULL) {
		if (QUITE_MODE>=1) {
			perror("opendir");
			std::fprintf(stderr, "Error: opendir %s\n", dname);
		}
		return false;
    }
    struct dirent *file;
    bool error=false;
    while((errno=0, file =readdir(dir)) != NULL) {
		struct stat statbuf;
		if (stat(file->d_name, &statbuf)==-1) {
			if (QUITE_MODE>=1) {		
				perror("stat");
				std::fprintf(stderr, "Error: stat %s\n", file->d_name);
			}
			return false;
		}
		if(S_ISDIR(statbuf.st_mode)) {
			if ( !isdot(file->d_name) ) {				
				if (walkDir(file->d_name, comp)) {
					if (chdir("..") == -1) {
						perror("chdir");
						std::fprintf(stderr, "Error: chdir ..\n");
						error = true;
					}
				}
				else error  = true;
			}
		} else {
			if (discardIt(file->d_name, comp)) {
				if (QUITE_MODE>=2){
					if (comp) {
						std::fprintf(stderr, "%s has already a %s suffix -- ignored\n", file->d_name, SUFFIX);
					} else {
						std::fprintf(stderr, "%s does not have a %s suffix -- ignored\n", file->d_name, SUFFIX);
					}
				}
				continue;									
			}
			if (!doWork(file->d_name, statbuf.st_size, comp)) error = true;
		}
    }
    if (errno != 0) {
		if (QUITE_MODE>=1) perror("readdir");
		error=true;
    }
    closedir(dir);
    return !error;
}

// ==================== OLD DEFINITIONS ====================


// NOT USED, OLD VERSION
// compression of a chunk of data
static inline bool compressDataChunked(unsigned char* ptr, size_t size, const std::string& fname) {
    
    // Compute how many 1MB chunks we'll need (round up)
    size_t n_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    // Buffer to hold compressed data
    std::vector<unsigned char*> out_ptr(n_chunks);
    std::vector<size_t>         out_sz(n_chunks);

    // Parallelize the compression of each chunk
    #pragma omp parallel for schedule(dynamic) num_threads(omp_get_max_threads())
    for (size_t i = 0; i < n_chunks; ++i) {
        size_t offset      = i * CHUNK_SIZE;
        size_t this_sz     = std::min(CHUNK_SIZE, size - offset);
        size_t bound       = compressBound(this_sz);
        unsigned char* buf = new unsigned char[bound];
        size_t comp_sz = bound;
        if (compress(buf, &comp_sz, ptr + offset, this_sz) != Z_OK) {
            delete[] buf;
            
        } else {
            out_ptr[i] = buf;
            out_sz[i]  = comp_sz;
        }
    }

    // Write all compressed chunks to the output file in series
    std::string outfile = fname + SUFFIX;
    std::ofstream out(outfile, std::ios::binary);
    if (!out.is_open()) return false;

    // header: magic + numero_chunk + original_size
    out.write(reinterpret_cast<const char*>(&CHUNKED_MAGIC), sizeof(CHUNKED_MAGIC));
    out.write(reinterpret_cast<const char*>(&n_chunks),      sizeof(n_chunks));
    out.write(reinterpret_cast<const char*>(&size),          sizeof(size));

    // each chunk: first its compressed size, then its data
    for (size_t i = 0; i < n_chunks; ++i) {
        out.write(reinterpret_cast<const char*>(&out_sz[i]), sizeof(out_sz[i]));
        out.write(reinterpret_cast<const char*>(out_ptr[i]), out_sz[i]);
        delete[] out_ptr[i];
    }
    out.close();

    if (REMOVE_ORIGIN) unlink(fname.c_str());
    return true;
}

// NOT USED, OLD VERSION
// decompress a chunk of data
static inline bool decompressDataChunked(unsigned char *ptr, size_t size, const std::string &fname) {
   
    // header: magic + numero_chunk + original_size
    if (size < sizeof(uint32_t) + 2*sizeof(size_t)) return false;
    //uint32_t magic           = *reinterpret_cast<uint32_t*>(ptr);
    //(void)magic;
    ptr  += sizeof(uint32_t);
    size_t n_chunks          = *reinterpret_cast<size_t*>(ptr);
    ptr  += sizeof(size_t);
    size_t original_size     = *reinterpret_cast<size_t*>(ptr);
    ptr  += sizeof(size_t);

    // create and memory-map the output file at its exact original size
    std::string outfile = fname.substr(0, fname.size() - strlen(SUFFIX));
    unsigned char *outPtr = nullptr;
    if (!allocateFile(outfile.c_str(), original_size, outPtr)) return false;

    // decompress each chunk in parallel
    for (size_t i = 0; i < n_chunks; ++i) {
        size_t comp_sz     = *reinterpret_cast<size_t*>(ptr);
        ptr               += sizeof(size_t);
        size_t offset      = i * CHUNK_SIZE;
        size_t chunk_uncomp = std::min(CHUNK_SIZE, original_size - offset);
        if (uncompress(outPtr + offset, &chunk_uncomp, ptr, comp_sz) != Z_OK) {
            unmapFile(outPtr, original_size);
            return false;
        }
        ptr += comp_sz;
    }

    unmapFile(outPtr, original_size);
    if (REMOVE_ORIGIN){
         unlink(fname.c_str());
    }
    return true;
}

// NOT USED, OLD VERSION
// entry-point for compression and decompression 
// returns false in case of error
static inline bool doParallelWork(const char fname[], size_t size, const bool comp) {
    unsigned char *ptr = nullptr;
    if (!mapFile(fname, size, ptr)) {
		if (QUITE_MODE>=1) 
			std::fprintf(stderr, "mapFile %s failed\n", fname);
		return false;
	}
    bool r;
    if (comp) {
        if (size < CHUNK_SIZE) {
            r = compressData(ptr, size, fname);
        } else {
            r = compressDataChunked(ptr, size, fname);
        }
    } else {
        // check if the file is chunked
        uint32_t magic = *reinterpret_cast<uint32_t*>(ptr);
        //(void)magic;
        if (magic == CHUNKED_MAGIC)
            r = decompressDataChunked(ptr, size, fname);
        else
            r = decompressData(ptr, size, fname);
    }

    unmapFile(ptr, size);
	return r;
}


//===================== END OLD DEFINITIONS ====================

//===================== MY DEFINITIONS ====================

// compressParallelFile: performs chunked compression of a single file using OpenMP tasks
static inline bool compressParallelFile(const char *fname) {

     // Determine file size and check if it's a directory 
    size_t size = 0;
    if (isDirectory(std::filesystem::path(fname), size)) {
        // Verbose mode: report directories
        if (QUITE_MODE>=1) 
            std::fprintf(stderr, "processFile %s failed because is a directory\n", fname);
        return false;
    }

    // Memory-map the input file for zero-copy access
    unsigned char *ptr = nullptr;
    if (!mapFile(fname, size, ptr)) {
        // Verbose mode: report mapping errors
        if (QUITE_MODE>=1) 
            std::fprintf(stderr, "mapFile %s failed\n", fname);
        return false;
    }

    // Compute how many 1MB chunks we'll need (round up)
    size_t n_chunks = size / CHUNK_SIZE;
    // Prepare vectors to hold pointers and sizes for each compressed chunk
    std::vector<unsigned char*> out_ptr(n_chunks);
    std::vector<size_t>         out_sz(n_chunks);

    // Launch an OpenMP task for each chunk within a taskgroup
    // This allows nested parallelism: chunk tasks can run in parallel
    #pragma omp taskgroup
    {
        for (size_t i = 0; i < n_chunks; ++i) {
            // Each task firstprivate(i, size, fname, ptr) to capture necessary variables
            #pragma omp task firstprivate(i, size, fname, ptr) shared(out_ptr, out_sz)
            {
                // Calculate chunk boundaries
                size_t offset  = i * CHUNK_SIZE;
                size_t this_sz = std::min(CHUNK_SIZE, size - offset);
                // Estimate maximum compressed size
                size_t bound   = compressBound(this_sz);

                // Allocate temporary buffer for compressed data
                unsigned char *buf = new unsigned char[bound];
                size_t comp_sz     = bound;

                // Perform in-memory compression of this chunk
                compress(buf, &comp_sz, ptr + offset, this_sz);

                // Store results in shared arrays
                out_ptr[i] = buf;
                out_sz[i]  = comp_sz;
            }
        }
    }

    // Write all compressed chunks to the output file in series
    std::string outfile = std::string(fname) + SUFFIX;
    std::ofstream out(outfile, std::ios::binary);
    if (!out.is_open()) {
        // Verbose mode: report files not opened
        if (QUITE_MODE >= 1) 
            std::fprintf(stderr, "Failed to open output file: %s\n", outfile.c_str());
        for (auto p : out_ptr) delete[] p;
        unmapFile(ptr, size);
        return false;
    }

    // Write header: number of chunks + original size
    out.write(reinterpret_cast<const char*>(&n_chunks),      sizeof(n_chunks));
    out.write(reinterpret_cast<const char*>(&size),          sizeof(size));

    // Serialize each chunk: first its compressed size, then its data
    for (size_t i = 0; i < n_chunks; ++i) {
        out.write(reinterpret_cast<const char*>(&out_sz[i]), sizeof(out_sz[i]));
        out.write(reinterpret_cast<const char*>(out_ptr[i]), out_sz[i]);
        delete[] out_ptr[i];
    }
    out.close();

    // Clean up memory mappings and optionally remove the original file
    unmapFile(ptr, size);
    if (REMOVE_ORIGIN) {
        unlink(fname);
    }

    return true;
}

// decompressParallelFile: performs chunked decompression of a single file using OpenMP tasks
static inline bool decompressParallelFile(const char *fname) {

    // Memory-map the input compressed file
    size_t size = 0;
    unsigned char *basePtr = nullptr;
    if (!mapFile(fname, size, basePtr)) {
        // Verbose mode: report mapping errors
        if (QUITE_MODE>=1)
            std::fprintf(stderr, "mapFile %s failed\n", fname);
        return false;
    }
    unsigned char *ptr = basePtr;

    // Read header: number of chunks + original size
    size_t n_chunks    = *reinterpret_cast<size_t*>(ptr);
    ptr += sizeof(n_chunks);
    size_t orig_size   = *reinterpret_cast<size_t*>(ptr);
    ptr += sizeof(orig_size);

    // Create and memory-map the output file at its exact original size
    std::string outfile = std::string(fname).substr(0, std::strlen(fname) - std::strlen(SUFFIX));
    unsigned char *outPtr = nullptr;
    if (!allocateFile(outfile.c_str(), orig_size, outPtr)) {
        // Verbose error reporting
        if (QUITE_MODE >= 1)
            std::fprintf(stderr, "Failed to allocate output file: %s\n", outfile.c_str());
        // Clean up the input mapping
        unmapFile(basePtr, size);
        return false;
    }

    //  Read per-chunk metadata into two arrays:
    //    chunk_sz[i]  = compressed size of chunk i
    //    chunk_ptr[i] = pointer to chunk i’s data in the mapped input
    std::vector<unsigned char*> chunk_ptr(n_chunks);
    std::vector<size_t>         chunk_sz(n_chunks);
    for (size_t i = 0; i < n_chunks; ++i) {
        chunk_sz[i]   = *reinterpret_cast<size_t*>(ptr);
        ptr          += sizeof(chunk_sz[i]);
        chunk_ptr[i]  = ptr;
        ptr          += chunk_sz[i];
    }

    // Launch an OpenMP taskgroup to decompress each chunk in parallel
    #pragma omp taskgroup
    for (size_t i = 0; i < n_chunks; ++i) {
        #pragma omp task firstprivate(i) shared(chunk_ptr, chunk_sz, outPtr, orig_size)
        {
            // Compute where in the output this chunk should land
            size_t offset = i * CHUNK_SIZE;
            size_t this_uncomp = std::min(CHUNK_SIZE, orig_size - offset);
            // Decompress directly into the mapped output buffer
            uncompress(outPtr + offset, &this_uncomp,
                       chunk_ptr[i], chunk_sz[i]);
        }
    }

    // Cleanup: unmap both mappings (which also flushes the file)
    unmapFile(outPtr, orig_size);
    unmapFile(basePtr, size);
    if (REMOVE_ORIGIN) {
        unlink(fname);
    } 
    return true;
}

// ==================== END MY DEFINITIONS ====================


#endif  // _UTILITY_HPP
