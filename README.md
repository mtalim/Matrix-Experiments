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
Block: 216KiB  Matrix: 31104KiB  Dim: 576x2304x8B\
mat_mul_basic   6.73906s\
mat_mul_trans   1.59309s (323.018% faster)\
mat_mul_outer   1.5109s (346.029% faster)\
mat_mul_cb      1.56221s (331.378% faster)\
mat_mul_avx     1.57897s (326.802% faster)\
mat_mul_cb_avx  1.48891s (352.617% faster)\

Block: 216KiB  Matrix: 31104KiB  Dim: 2304x576x8B\
mat_mul_basic   23.6424s\
mat_mul_trans   6.49252s (264.148% faster)\
mat_mul_outer   6.05718s (290.32% faster)\
mat_mul_cb      6.29285s (275.702% faster)\
mat_mul_avx     9.25121s (155.56% faster)\
mat_mul_cb_avx  5.97142s (295.926% faster)\
