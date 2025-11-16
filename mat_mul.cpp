#include <iostream>
#include <numeric>
#include <functional>
#include <random>
#include <limits>
#include <chrono>
#include <utility>
#include <cassert>

template <typename T>
class Matrix
{
private:
    T *matrix;
    unsigned row, column;
public:
    Matrix() : matrix(nullptr), row(0), column(0)
    {
    }
    Matrix(unsigned row, unsigned column)
    {
        this->matrix = new T[row*column];
        this->row = row;
        this->column = column;
    }
    Matrix(const Matrix& other) : Matrix(other.row, other.column)
    {
        for (auto i = 0; i < (row*column); i++)
            this->matrix[i] = other.matrix[i];
    }
    Matrix(Matrix&& other) : matrix(other.matrix), row(other.row), column(other.column)
    {
        other.matrix = nullptr;
        other.row = 0;
        other.column = 0;
    }

    ~Matrix()
    {
        delete[] matrix;
    }

    Matrix& operator=(const Matrix& other)
    {
        if (this != &other)
        {
            if (this->row != other.row || this->column != other.column)
            {
                this->~Matrix();

                this->row = other.row;
                this->column = other.column;

                this->matrix = new T[row*column];
            }

            for (auto i = 0; i < (row*column); i++)
                this->matrix[i] = other.matrix[i];
        }
        return *this;
    }
    Matrix& operator=(Matrix&& other)
    {
        if (this != &other)
        {
            this->~Matrix();

            this->matrix = other.matrix;
            this->row = other.row;
            this->column = other.column;

            other.row = 0;
            other.column = 0;
            other.matrix = nullptr;
        }

        return *this;
    }

    void display() const
    {
        for (auto r = 0; r < row; r++)
        {
            T* A = (*this)[r];
            for (auto c = 0; c < column; c++)
            {
                std::cout<< A[c] << ' ';
            }
            std::cout<< std::endl;
        }
    }

    void rand_fill(std::function<T()> gen_rand)
    {
        for (auto i = 0; i < (row*column); i++)
            matrix[i] = gen_rand();
    }

    T* operator[](unsigned r) const
    {
        return matrix + (r*column);
    }

    bool operator==(const Matrix& other) const
    {
        if (row != other.row || column != other.column)
            return false;

        for (auto i = 0; i < (row*column); i++)
            if (this->matrix[i] != other.matrix[i])
                    return false;

        return true;
    }

    static bool multipliable(const Matrix& a, const Matrix& b)
    {
        return a.column == b.row;
    }

    // Matrix operator*(const Matrix& b) const

    static Matrix mat_mul_basic(const Matrix& a, const Matrix& b)
    {
        assert(Matrix::multipliable(a, b));
        const auto res_row = a.row;
        const auto res_col = b.column;
        const auto vec_len = a.column; // same as b.row

        Matrix<T> result(res_row, res_col);
        for (auto r = 0; r < res_row; r++)
        {
            T* A = a[r];
            T* C = result[r];
            for (auto c = 0; c < res_col; c++)
            {
                C[c] = 0;

                for (auto i = 0; i < vec_len; i++)
                {
                    C[c] += A[i] * b[i][c];
                }
            }
        }
        return result;
    }

    static Matrix mat_mul_trans(const Matrix& a, const Matrix& b)
    {
        assert(Matrix::multipliable(a, b));
        const auto res_row = a.row;
        const auto res_col = b.column;
        const auto vec_len = a.column; // same as b.row

        Matrix<T> result(res_row, res_col);
        T vec_b[vec_len]; ///\TODO -pendatic compile flag doesn't like stack variable-length arrays; not ISO C++ compliant
        for (auto c = 0; c < res_col; c++)
        {
            for (auto e = 0; e < vec_len; e++)
                vec_b[e] = b[e][c];

            for (auto r = 0; r < res_row; r++)
            {
                T* A = a[r];
                T* C = result[r];
                C[c] = 0;

                for (auto i = 0; i < vec_len; i++)
                {
                    C[c] += A[i] * vec_b[i];
                }
            }
        }
        return result;
    }


    static Matrix mat_mul_outer(const Matrix& a, const Matrix& b)
    {
        assert(Matrix::multipliable(a, b));
        const auto res_row = a.row;
        const auto res_col = b.column;
        const auto vec_len = a.column; // same as b.row

        Matrix<T> result(res_row, res_col);
        for (auto r = 0; r < res_row; r++)
        {
            T* C = result[r];
            for (auto c = 0; c < res_col; c++)
                C[c] = 0;
        }

        for (auto r = 0; r < res_row; r++)
        {
            T* C = result[r];

            for (auto i = 0; i < vec_len; i++)
            {
                T A = a[r][i];
                T* B = b[i];
                for (auto c = 0; c < res_col; c++)
                {
                    C[c] += A * B[c];
                }
            }
        }
        return result;
    }

    static Matrix mat_mul_avx(const Matrix& a, const Matrix& b)
    {
        assert(Matrix::multipliable(a, b));
        const auto res_row = a.row;
        const auto res_col = b.column;
        const auto vec_len = a.column; // same as b.row

        Matrix<T> result(res_row, res_col);
        T vec_b[vec_len];
        for (auto c = 0; c < res_col; c++)
        {
            for (auto e = 0; e < vec_len; e++)
                vec_b[e] = b[e][c];

            for (auto r = 0; r < res_row; r++)
            {
                result[r][c] = std::inner_product((a[r]), (a[r])+vec_len, vec_b, static_cast<T>(0));
            }
        }
        return result;
    }

    template <unsigned BLK_DIM>
    static Matrix mat_mul_cb(const Matrix& a, const Matrix& b)
    {
        assert(Matrix::multipliable(a, b));
        const auto res_row = a.row;
        const auto res_col = b.column;
        const auto vec_len = a.column; // same as b.row

        Matrix<T> result(res_row, res_col);
        for (auto r = 0; r < res_row; r++)
        {
            T* C = result[r];
            for (auto c = 0; c < res_col; c++)
                C[c] = 0;
        }

        // C matrix
        for (auto r = 0; r < res_row; r+=BLK_DIM)
        {
            for (auto c = 0; c < res_col; c+=BLK_DIM)
            {
                /** For 3x3 block element mults accesses:
                 *  k > i > j: 7r14c  = 3r6c + 3r   + 1r8c
                 *  i > k > j: 5r16c  = 1r8c + 1r2c + 3r6c (but takes more housekeeping overhead to keep k "blocked")
                 *  i > j > k: 11r10c = 1r2c + 1r8c + 9r
                 *
                 *  Cij += Aik * Bkj
                 */

                // Blocks
                // for (auto b = 0; b < res_row; b+=BLK_DIM)
                for (auto k = 0; k < vec_len ; k++)
                {
                    T* B = b[k];
                    // Partial products
                    for (auto i = r; i < r + BLK_DIM; i++)
                    {
                        T* C = result[i];
                        T A = a[i][k];
                        for (auto j = c; j < c + BLK_DIM; j++)
                        // Vector product
                            C[j] += A * B[j];
                    }
                }
            }
        }
        return result;
    }

    template <unsigned BLK_DIM>
    static Matrix mat_mul_cb_avx(const Matrix& a, const Matrix& b)
    {
        assert(Matrix::multipliable(a, b));
        const auto res_row = a.row;
        const auto res_col = b.column;
        const auto vec_len = a.column; // same as b.row

        Matrix<T> result(res_row, res_col);
        T vec_b[BLK_DIM][vec_len];

        for (auto r = 0; r < res_row; r++)
        {
            T* C = result[r];
            for (auto c = 0; c < res_col; c++)
                C[c] = 0;
        }

        // C matrix
        for (auto c = 0; c < res_col; c+=BLK_DIM)
        {
            for (auto f = 0; f < BLK_DIM; f++)
            for (auto e = 0; e < vec_len; e++)
                vec_b[f][e] = b[e][c+f];

            for (auto r = 0; r < res_row; r+=BLK_DIM)
            {
                // Blocks
                // for (auto b = 0; b < row; b+=BLK_DIM)
                // Partial products
                for (auto i = r; i < r + BLK_DIM; i++)
                {
                    T* A = a[i];
                    for (auto j = c; j < c + BLK_DIM; j++)
                        result[i][j] = std::inner_product(A, A+vec_len, (vec_b[j-c]), result[i][j]);
                }
            }
        }
        return result;
    }
};


void time_run(std::function<void()> func)
{
    static double baseline = 0;

    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double>(end - start).count();

    std::cout<< duration << "s";
    if (baseline)
        std::cout<< " (" << (baseline/duration - 1)*100 << "% faster)";
    else
        baseline = duration;
    std::cout<< std::endl;
}

int main()
{
    using my_t = double;
    constexpr unsigned DIM_SCALE_X = 24;
    constexpr unsigned DIM_SCALE_Y = 6;
    constexpr unsigned BLK_DIM = 96;
    constexpr unsigned DIM_X = BLK_DIM * DIM_SCALE_X;
    constexpr unsigned DIM_Y = BLK_DIM * DIM_SCALE_Y;

    Matrix<my_t> mat_a(DIM_Y,DIM_X), mat_b(DIM_X,DIM_Y), mat_x(DIM_Y,DIM_X), mat_y(DIM_X,DIM_Y);

    std::mt19937_64 rand_gen;
    std::uniform_int_distribution<uint64_t> dist(std::numeric_limits<uint64_t>::min(),
                                                 std::numeric_limits<uint64_t>::max());
    auto gen_rand = [&rand_gen, &dist]() -> uint64_t { return dist(rand_gen); };

    mat_a.rand_fill(gen_rand);
    mat_b.rand_fill(gen_rand);
    mat_x.rand_fill(gen_rand);
    mat_y.rand_fill(gen_rand);

    std::pair<const char *, std::function<Matrix<my_t>(Matrix<my_t>&, Matrix<my_t>&)>> funcs[] = {
        {"mat_mul_basic",  Matrix<my_t>::mat_mul_basic},
        {"mat_mul_trans",  Matrix<my_t>::mat_mul_trans},
        {"mat_mul_outer",  Matrix<my_t>::mat_mul_outer},
        {"mat_mul_cb",     Matrix<my_t>::mat_mul_cb<BLK_DIM>},
        {"mat_mul_avx",    Matrix<my_t>::mat_mul_avx},
        {"mat_mul_cb_avx", Matrix<my_t>::mat_mul_cb_avx<BLK_DIM>},
    };

    auto cache_warmup_clearing =
        [&mat_a, &mat_b](Matrix<my_t>& mat){ mat = std::move(Matrix<my_t>::mat_mul_avx(mat_a, mat_b)); };



    std::cout<< "Block: " << (BLK_DIM*BLK_DIM * 3*sizeof(my_t)/1024)
             << "KiB  Matrix: " << (DIM_X*DIM_Y * 3*sizeof(my_t)/1024)
             << "KiB  Dim: " << DIM_Y << "x" << DIM_X << "x" << sizeof(my_t) << "B\n";

    Matrix<my_t> prev_mat;
    bool subsequent_run = false;
    for (auto [name, func] : funcs)
    {
        Matrix<my_t> mat;
        std::cout<< name << "\t";
        cache_warmup_clearing(mat);
        time_run([&](){mat = std::move(func(mat_x, mat_y));});

        // Check for consistent results
        if (subsequent_run) assert(mat == prev_mat);
        subsequent_run = true;
        prev_mat = std::move(mat);
    }

    ///\TODO mat_mul_outer_avx - using elementwise AVX _mm256_mullo_epi32

    // mat_x.display(); std::cout<< std::endl;
    // mat_y.display(); std::cout<< std::endl;
    // prev_mat.display(); std::cout<< std::endl;

    return 0;
}