#include <iostream>
#include <random>
#include <chrono>
#include <limits>
#include <functional>
#include <numeric>

using element_type = int;
constexpr unsigned DIM_SCALE = 16;
constexpr unsigned BLK_DIM = 64;
constexpr unsigned DIM = BLK_DIM * DIM_SCALE;



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
        // for (auto r = 0; r < row; r++)
        // {
        //     matrix[r] = new T[column];
        // }
        this->row = row;
        this->column = column;
    }
    Matrix(const Matrix& other) : Matrix(other.row, other.column)
    {
        for (auto r = 0; r < row; r++)
        {
            T* A = (*this)[r];
            T* B = other[r];
            for (auto c = 0; c < column; c++)
            {
                A[c] = B[c];
            }
        }
    }
    Matrix(Matrix&& other) : matrix(other.matrix), row(other.row), column(other.column)
    {
        other.matrix = nullptr;
        other.row = 0;
        other.column = 0;
    }

    ~Matrix()
    {
        // for (auto r = 0; r < row; r++)
        // {
        //     delete[] matrix[r];
        // }
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
                // for (auto r = 0; r < row; r++)
                // {
                //     matrix[r] = new T[column];
                // }
            }

            for (auto r = 0; r < row; r++)
            {
                T* A = (*this)[r];
                T* B = other[r];
                for (auto c = 0; c < column; c++)
                {
                    A[c] = B[c];
                }
            }
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

    void display()
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

    void rand_fill(std::function<int()> gen_rand)
    {
        for (auto r = 0; r < row; r++)
        {
            T* A = (*this)[r];
            for (auto c = 0; c < column; c++)
            {
                A[c] = gen_rand();
            }
        }
    }

    T* operator[](unsigned r) const
    {
        return matrix + (r*column);
    }

    bool operator==(const Matrix& other) const
    {
        if (row != other.row || column != other.column)
            return false;

        Matrix<T> result(row, column);
        for (auto r = 0; r < row; r++)
        {
            T* A = (*this)[r];
            T* B = other[r];
            for (auto c = 0; c < column; c++)
            {
                if (A[c] != B[c])
                    return false;
            }
        }

        return true;
    }

    Matrix operator*(const Matrix& other) const
    {
        Matrix<T> result(row, column);
        for (auto r = 0; r < row; r++)
        {
            T* A = (*this)[r];
            T* C = result[r];
            for (auto c = 0; c < column; c++)
            {
                C[c] = 0;

                for (auto i = 0; i < row; i++)
                {
                    C[c] += A[i] * other[i][c];
                }
            }
        }
        return result;
    }

    Matrix mat_mul_trans(const Matrix& other) const
    {
        Matrix<T> result(row, column);
        T vec_b[row]; ///\TODO -pendatic compile flag doesn't like stack variable-length arrays; not ISO C++ compliant
        for (auto c = 0; c < column; c++)
        {
            for (auto e = 0; e < row; e++)
                vec_b[e] = other[e][c];

            for (auto r = 0; r < row; r++)
            {
                T* A = (*this)[r];
                T* C = result[r];
                C[c] = 0;

                for (auto i = 0; i < row; i++)
                {
                    C[c] += A[i] * vec_b[i];
                }
            }
        }
        return result;
    }


    Matrix mat_mul_outer(const Matrix& other) const
    {
        Matrix<T> result(row, column);

        for (auto r = 0; r < row; r++)
        {
            T* C = result[r];
            for (auto c = 0; c < column; c++)
                C[c] = 0;
        }

        for (auto r = 0; r < row; r++)
        {
            T* C = result[r];

            for (auto i = 0; i < row; i++)
            {
                T A = (*this)[r][i];
                T* B = other[i];
                for (auto c = 0; c < column; c++)
                {
                    C[c] += A * B[c];
                }
            }
        }
        return result;
    }

    Matrix mat_mul_avx(const Matrix& other) const
    {
        Matrix<T> result(row, column);
        T vec_b[row];
        for (auto c = 0; c < column; c++)
        {
            for (auto e = 0; e < row; e++)
                vec_b[e] = other[e][c];

            for (auto r = 0; r < row; r++)
            {
                result[r][c] = std::inner_product(((*this)[r]), ((*this)[r])+column, vec_b, 0);
            }
        }
        return result;
    }

    Matrix mat_mul_cb(const Matrix& other) const
    {
        Matrix<T> result(row, column);

        for (auto r = 0; r < row; r++)
        {
            T* C = result[r];
            for (auto c = 0; c < column; c++)
                C[c] = 0;
        }

        // C matrix
        for (auto r = 0; r < row; r+=BLK_DIM)
        {
            for (auto c = 0; c < column; c+=BLK_DIM)
            {
                /** For 3x3 block element mults accesses:
                 *  k > i > j: 7r14c  = 3r6c + 3r   + 1r8c
                 *  i > k > j: 5r16c  = 1r8c + 1r2c + 3r6c (but takes more housekeeping overhead to keep k "blocked")
                 *  i > j > k: 11r10c = 1r2c + 1r8c + 9r
                 *
                 *  Cij += Aik * Bkj
                 */

                // Blocks
                // for (auto b = 0; b < row; b+=BLK_DIM)
                for (auto k = 0; k < row ; k++)
                {
                    T* B = other[k];
                    // Partial products
                    for (auto i = r; i < r + BLK_DIM; i++)
                    {
                        T* C = result[i];
                        T A = (*this)[i][k];
                        for (auto j = c; j < c + BLK_DIM; j++)
                        // Vector product
                            C[j] += A * B[j];
                    }
                }
            }
        }
        return result;
    }

    Matrix mat_mul_cb_avx(const Matrix& other) const
    {
        Matrix<T> result(row, column);
        T vec_b[BLK_DIM][row];

        for (auto r = 0; r < row; r++)
        {
            T* C = result[r];
            for (auto c = 0; c < column; c++)
                C[c] = 0;
        }

        // C matrix
        for (auto c = 0; c < column; c+=BLK_DIM)
        {
            for (auto f = 0; f < BLK_DIM; f++)
            for (auto e = 0; e < row; e++)
                vec_b[f][e] = other[e][c+f];

            for (auto r = 0; r < row; r+=BLK_DIM)
            {
                // Blocks
                // for (auto b = 0; b < row; b+=BLK_DIM)
                // Partial products
                for (auto i = r; i < r + BLK_DIM; i++)
                {
                    T* A = (*this)[i];
                    for (auto j = c; j < c + BLK_DIM; j++)
                        result[i][j] = std::inner_product(A, A+row, (vec_b[j-c]), result[i][j]);
                }
            }
        }
        return result;
    }
};


void time_run(std::function<void()> func, bool reset_baseline = false)
{
    static double baseline;

    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double>(end - start).count();

    std::cout<< duration << "s";
    if (reset_baseline)
    {
        baseline = duration;
    }
    else
        std::cout<< " (" << (baseline/duration - 1)*100 << "% faster)";
    std::cout<< std::endl;
}

int main()
{
    Matrix<element_type> mat_a(DIM,DIM), mat_b(DIM,DIM), mat_x(DIM,DIM), mat_y(DIM,DIM);

    std::mt19937 rand_gen;
    std::uniform_int_distribution<> dist(std::numeric_limits<element_type>::min(),
                                         std::numeric_limits<element_type>::max());

    mat_a.rand_fill([&rand_gen, &dist](){return dist(rand_gen);});
    mat_b.rand_fill([&rand_gen, &dist](){return dist(rand_gen);});
    mat_x.rand_fill([&rand_gen, &dist](){return dist(rand_gen);});
    mat_y.rand_fill([&rand_gen, &dist](){return dist(rand_gen);});

    std::cout<< "Block: " << (BLK_DIM*BLK_DIM * 3*sizeof(element_type)/1024)
             << "KiB  Matrix: " << (DIM*DIM * 3*sizeof(element_type)/1024) << "KiB\n";

    Matrix<element_type> mat[5];

    auto cache_warmup_clearing =
        [&mat_a, &mat_b](Matrix<element_type>& mat){ mat = std::move(mat_a.mat_mul_avx(mat_b)); };

    std::cout<< "mat_x * mat_y\t";
    cache_warmup_clearing(mat[0]);
    time_run([&](){mat[0] = std::move(mat_x * mat_y);}, true);

    std::cout<< "mat_mul_trans\t";
    cache_warmup_clearing(mat[1]);
    time_run([&](){mat[1] = std::move(mat_x.mat_mul_trans(mat_y));});

    std::cout<< "mat_mul_cb\t";
    cache_warmup_clearing(mat[2]);
    time_run([&](){mat[2] = std::move(mat_x.mat_mul_cb(mat_y));});

    std::cout<< "mat_mul_avx\t";
    cache_warmup_clearing(mat[3]);
    time_run([&](){mat[3] = std::move(mat_x.mat_mul_avx(mat_y));});

    std::cout<< "mat_mul_cb_avx\t";
    cache_warmup_clearing(mat[4]);
    time_run([&](){mat[4] = std::move(mat_x.mat_mul_cb_avx(mat_y));});

    ///\TODO mat_mul_outer_avx - using elementwise AVX _mm256_mullo_epi32


    // mat[1]].display();
    // std::cout<< std::endl;

    // check equality
    std::cout<< (mat[0] == mat[1])
             << (mat[0] == mat[2])
             << (mat[0] == mat[3])
             << (mat[0] == mat[4])
             << std::endl;

    return 0;
}