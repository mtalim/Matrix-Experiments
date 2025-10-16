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
* Support non-square matrices
* -pendatic compiler flag doesn't like stack variable-length arrays; not ISO C++-compliant

Example runs:\
BLK_DIM = 64; DIM_SCALE = 16;\
Block: 48KiB  Matrix: 12288KiB\
mat_x * mat_y   6.33452s\
mat_mul_trans   2.18891s (189.392% faster)\
mat_mul_cb      2.16797s (192.187% faster)\
mat_mul_avx     1.76828s (258.231% faster)\
mat_mul_cb_avx  1.73681s (264.722% faster)\

BLK_DIM = 64; DIM_SCALE = 12;\
Block: 48KiB  Matrix: 6912KiB\
mat_x * mat_y   2.66931s\
mat_mul_trans   1.01581s (162.776% faster)\
mat_mul_cb      0.99345s (168.691% faster)\
mat_mul_avx     0.797958s (234.517% faster)\
mat_mul_cb_avx  0.795014s (235.756% faster)\
