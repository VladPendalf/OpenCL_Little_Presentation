#include <iostream>
#include <immintrin.h> //интринксики -> on
#include <fstream>     //файлы -> on
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <thread>
#include <limits>

size_t vector_size = 100'000'000;

class Lab1
{
public:
    Lab1()
    {
    }
    void Payload_1(std::vector<float>& a,
                   std::vector<float>& b,
                   std::vector<float>& c)
    {
        const auto start = std::chrono::steady_clock::now();

        ///Разработать функцию, для нахождения поэлементного сложения двух массивов чисел типа float.
        for (int i = 0; i < vector_size; i += 4)
        {
            _mm_storeu_ps(&c[i], _mm_add_ps(_mm_loadu_ps(&a[i]),
                                            _mm_loadu_ps(&b[i])));
        }

        const auto end = std::chrono::steady_clock::now();
        std::cout << "\nВремя работы = " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << '\n';

        int i = 0;
        for (auto& it : a)
        {
            std::cout << '\n' << i << ") " << it;
            ++i;
        }
    }
};

void ReadToIn(std::string inputFilename,
              std::vector<float>* vecToData)
{
    std::ifstream in(inputFilename);

    if (!in.is_open())
        std::cout << inputFilename << " not open";

    auto size = vecToData->size();
    vecToData->insert(vecToData->begin(),
                      std::istream_iterator<float>(in),
                      std::istream_iterator<float>());
}

int main()
{
    setlocale(LC_ALL, "Ru-ru");
    std::cout << std::fixed << std::setprecision(9) << std::left;

    srand(time(0));
    auto C = std::vector<float>(vector_size, 1);
    auto A = std::vector<float>(vector_size, 1);
    auto B = std::vector<float>(vector_size, 1);

    std::thread thrA(ReadToIn, "SIMD_A2.txt", &A),
        thrB(ReadToIn, "SIMD_B2.txt", &B);
    thrA.join();
    thrB.join();

    Lab1 lab1;
    lab1.Payload_1(A, B, C);

}