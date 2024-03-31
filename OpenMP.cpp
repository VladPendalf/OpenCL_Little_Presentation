#include <iostream>
#include <chrono>
#include <omp.h>
#include <vector>
#include <random>

int64_t size = 100'000'000;

void GenerateRandVector(std::vector<float>& v)
{
    int vector_size = v.size();
    for (int i = 0; i < vector_size; i++)
    {
        v[i] = ((rand() % 200001) - 10000);
    }
    std::cout << "\nGen End";
}

//3'209'330'304

int main()
{
    srand(time(0));
    int i;
    std::vector<float> a, b, c;
    a.resize(size);
    b.resize(size);
    c.resize(size);
    GenerateRandVector(a);
    GenerateRandVector(b);
    auto begin = std::chrono::steady_clock::now();
#pragma omp parallel for
    for (i = 0; i < size; ++i)
    {
        c[i] = a[i] + b[i];
    }
    auto end = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "\nWork Time = " << diff_time.count() << " microseconds";
}