Experimented with:
* Vector/SIMD/AVX library instructions
    - Making code auto-vectorization friendly
    - Using known-good library functions
* Transposition of column to row
    - Allows for AVX instructions
* Loop Blocking/Tiling for cache locality
    - Tiling: 54 loads to complete 9 result elements in 3x3 (81 mults)
* Row access prioritization by loop interchanging for cache line utilization and prefetch
    - Row-pri: 90 loads to complete 81 mults(9 results)
    - Big gains! Even beats column-to-row transposition.
* 2D array flattening
    - Avoids array of array dynamic allocation
    - Less run-to-run variation seemingly
    - Still need to compare if runs slowed down;
* Manual "row" pointer intermediate saving
    - Avoids array of array dynamic allocation
    - Yielded big gains all around
    - Probably caused by operator[] after 2D flattening

TODOs:
* mat_mul_outer_avx - using elementwise AVX _mm256_mullo_epi32
* Automated graph and stats of multiple runs
* Support partial block remainders
* -pendatic compiler flag doesn't like stack variable-length arrays; not ISO C++-compliant

Example runs:\
g++ mat_mul.cpp -std=c++11 -DNDEBUG -O3

Block: 216KiB  Matrix: 23328KiB  Dim: 576x2304x8B
| Algo           | Run Time  | Relative Performance |
|----------------|-----------|----------------------|
| mat_mul_basic  | 3.91783s  |                      |
| mat_mul_trans  | 0.704344s | 456.238% faster      |
| mat_mul_outer  | 0.311173s | 1159.05% faster      |
| mat_mul_cb     | 0.204791s | 1813.09% faster      |
| mat_mul_avx    | 0.679368s | 476.688% faster      |
| mat_mul_cb_avx | 0.625565s | 526.287% faster      |

Block: 216KiB  Matrix: 62208KiB  Dim: 2304x576x8B
| Algo           | Run Time  | Relative Performance |
|----------------|-----------|----------------------|
| mat_mul_basic  | 4.8989s   |                      |
| mat_mul_trans  | 2.75747s  | 77.659% faster       |
| mat_mul_outer  | 1.03353s  | 373.995% faster      |
| mat_mul_cb     | 0.742521s | 559.766% faster      |
| mat_mul_avx    | 2.58337s  | 89.6323% faster      |
| mat_mul_cb_avx | 2.38049s  | 105.794% faster      |
