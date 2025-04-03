#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>      
#include <hpc_helpers.hpp>
#include <avx_mathfun.h>

// Sum of the elements of a vector
inline float hsum_sse3(__m128 v) {
	__m128 shuf = _mm_movehdup_ps(v);
	__m128 maxs = _mm_add_ps(v, shuf);
	shuf = _mm_movehl_ps(shuf, maxs);
	maxs = _mm_add_ss(maxs, shuf);
	return _mm_cvtss_f32(maxs);
} 


inline float hsum_avx(__m256 v) {
	__m128 lo = _mm256_castps256_ps128(v); 
	__m128 hi = _mm256_extractf128_ps(v, 1);
	lo = _mm_add_ps(lo, hi);
	return hsum_sse3(lo); 
}

// Maximum of the elements of a vector
inline float hmax_sse3(__m128 v) {
	__m128 shuf = _mm_movehdup_ps(v);
	__m128 maxs = _mm_max_ps(v, shuf);
	shuf = _mm_movehl_ps(shuf, maxs);
	maxs = _mm_max_ss(maxs, shuf);
	return _mm_cvtss_f32(maxs);
}

inline float hmax_avx(__m256 v) {
	__m128 lo = _mm256_castps256_ps128(v); 
	__m128 hi = _mm256_extractf128_ps(v, 1);
	lo = _mm_max_ps(lo, hi);
	return hmax_sse3(lo); 
}

void softmax_avx(const float *input, float *output, size_t K) {

	// Assuming that K is greater than 8
	size_t Kminus7 = K - 7;

    // Find the maximum to stabilize the computation of the exponential
	__m256 max_val = _mm256_set1_ps(std::numeric_limits<float>::lowest());
	for (size_t i = 0; i < Kminus7; i += 8) {
		__m256 input_val = _mm256_loadu_ps(input + i);
		max_val = _mm256_max_ps(max_val, input_val);
	}
	// Handle the case where K % 8 != 0
	max_val=_mm256_max_ps(_mm256_loadu_ps(input + K-8), max_val);

	// hmax_avx implementation
	float max = hmax_avx(max_val);

	// Computes all exponentials with the shift of max_val and the total sum
	__m256 sum_vec = _mm256_setzero_ps();
	__m256 max_input_vec = _mm256_set1_ps(max);
	for (size_t i = 0; i < Kminus7; i += 8) {
		__m256 input_val = _mm256_loadu_ps(input + i);
		__m256 exp_val = exp256_ps(_mm256_sub_ps(input_val, max_input_vec));
		_mm256_storeu_ps(output + i, exp_val);
		sum_vec = _mm256_add_ps(sum_vec, exp_val);
	}

	// Handle the case where K % 8 != 0 using masks
	if (K % 8 != 0) {
		size_t remainder_start = K - (K % 8);

		// Create a mask to indicate the position to consider
		__m256i mask = _mm256_set_epi32(
			(remainder_start + 7 < K) ? -1 : 0,
			(remainder_start + 6 < K) ? -1 : 0,
			(remainder_start + 5 < K) ? -1 : 0,
			(remainder_start + 4 < K) ? -1 : 0,
			(remainder_start + 3 < K) ? -1 : 0,
			(remainder_start + 2 < K) ? -1 : 0,
			(remainder_start + 1 < K) ? -1 : 0,
			(remainder_start + 0 < K) ? -1 : 0
		);

		int mask_array[8];
		_mm256_storeu_si256((__m256i*)mask_array, mask);

		__m256 input_val = _mm256_maskload_ps(input + remainder_start, mask);
		__m256 exp_val = exp256_ps(_mm256_sub_ps(input_val, max_input_vec));
		
		_mm256_maskstore_ps(output + remainder_start, mask, exp_val);

		// use the mask to indicate the correct elementes to be summed
		__m256 mask_sum = _mm256_and_ps(exp_val, _mm256_castsi256_ps(mask));
		sum_vec = _mm256_add_ps(sum_vec, mask_sum);  

		// Another way to use the mask using FMA (doesn't work on backends nodes
		// that don't support FMA)

		// __m256 mask_fma = _mm256_and_ps(_mm256_castsi256_ps(mask), _mm256_set1_ps(1.0f));
        // sum_vec = _mm256_fmadd_ps(exp_val, mask_fma, sum_vec);
	}

	// hsum_avx implementation
	float sum = hsum_avx(sum_vec);

	// Normalize by dividing for the total sum
	__m256 sum_vect = _mm256_set1_ps(sum);
	for (size_t i = 0; i < Kminus7; i += 8) {
		__m256 output_val = _mm256_loadu_ps(output + i);
		output_val = _mm256_div_ps(output_val, sum_vect);
		_mm256_storeu_ps(output + i, output_val);
	}

	// Handle the case where K % 8 != 0 using masks
	if (K % 8 != 0) {
		size_t remainder_start = K - (K % 8);
		__m256i mask = _mm256_set_epi32(
			(remainder_start + 7 < K) ? -1 : 0,
			(remainder_start + 6 < K) ? -1 : 0,
			(remainder_start + 5 < K) ? -1 : 0,
			(remainder_start + 4 < K) ? -1 : 0,
			(remainder_start + 3 < K) ? -1 : 0,
			(remainder_start + 2 < K) ? -1 : 0,
			(remainder_start + 1 < K) ? -1 : 0,
			(remainder_start + 0 < K) ? -1 : 0
		);
		__m256 output_val = _mm256_maskload_ps(output + remainder_start, mask);
		output_val = _mm256_div_ps(output_val, sum_vect);
		_mm256_maskstore_ps(output + remainder_start, mask, output_val);
	}
}

std::vector<float> generate_random_input(size_t K, float min = -1.0f, float max = 1.0f) {
    std::vector<float> input(K);
    //std::random_device rd;
    //std::mt19937 gen(rd());
	std::mt19937 gen(5489); // fixed seed for reproducible results
    std::uniform_real_distribution<float> dis(min, max);
    for (size_t i = 0; i < K; ++i) {
        input[i] = dis(gen);
    }
    return input;
}

void printResult(std::vector<float> &v, size_t K) {
	for(size_t i=0; i<K; ++i) {
		std::fprintf(stderr, "%f\n",v[i]);
	}
}


int main(int argc, char *argv[]) {
	if (argc == 1) {
		std::printf("use: %s K [1]\n", argv[0]);
		return 0;		
	}
	size_t K=0;
	if (argc >= 2) {
		K = std::stol(argv[1]);
	}
	bool print=false;
	if (argc == 3) {
		print=true;
	}	
	std::vector<float> input=generate_random_input(K);
	std::vector<float> output(K);

	TIMERSTART(softime_avx);
	softmax_avx(input.data(), output.data(), K);
	TIMERSTOP(softime_avx);
	
	// print the results on the standard output
	if (print) {
		printResult(output, K);
	}
}