#include <vector>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

size_t vector_size = 100'000'000;
size_t part = vector_size / 8;

//using namespace oneapi::tbb;

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

void VecSum(std::vector<float>* a,
            std::vector<float>* b,
            std::vector<float>* c,
            int range)
{
    for (int dist = range + part; range < dist; ++range)
    {
        (*c)[range] = (*a)[range] + (*b)[range];
    }
}

int main()
{
    std::vector<float> C(vector_size, 1);
    std::vector<float>A(vector_size, 1);
    std::vector<float>B(vector_size, 1);

    std::thread thrA(ReadToIn, "TBB_A2.txt", &A),
        thrB(ReadToIn, "TBB_B2.txt", &B);
    thrA.join();
    thrB.join();

    //for (int i = 0; i < vector_size; i++)
    //{
    //    A[i] = ((rand() % 200001) - 10000); //[-100'000:100'000]
    //    B[i] = ((rand() % 200001) - 10000);
    //}

    std::vector<std::thread> arr;
    int range = 0;

    for (int i = 0; i < 8; ++i)
    {
        arr.emplace_back(std::thread(VecSum, &A, 
                                     &B, 
                                     &C, 
                                     range));
        std::cout << "range = " << range << '\n';
        range += part;
    }
    auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < arr.size(); ++i)
    {
        arr[i].join();
    }
    auto end = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "\nTime = " << diff_time.count() << "microseconds\n";

    auto itStart = C.begin();
    auto itEnd = C.rbegin();
    std::cout << "first elem = " << *itStart << "\nLast elem = " << *itEnd;

    return 0;
}
