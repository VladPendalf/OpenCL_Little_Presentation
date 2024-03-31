//==============================================================
// Vector Add is the equivalent of a Hello, World! sample for data parallel
// programs. Building and running the sample verifies that your development
// environment is setup correctly and demonstrates the use of the core features
// of SYCL. This sample runs on both CPU and GPU (or FPGA). When run, it
// computes on both the CPU and offload device, then compares results. If the
// code executes on both CPU and offload device, the device name and a success
// message are displayed. And, your development environment is setup correctly!
//
// For comprehensive instructions regarding SYCL Programming, go to
// https://software.intel.com/en-us/oneapi-programming-guide and search based on
// relevant terms noted in the comments.
//
// SYCL material used in the code sample:
// •	A one dimensional array of data.
// •	A device queue, buffer, accessor, and kernel.
//==============================================================
// Copyright © Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
//#include <sycl/sycl.hpp>
#include <vector>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <oneapi/tbb.h>
#include <oneapi/tbb/parallel_for.h>
//#if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
//#include <sycl/ext/intel/fpga_extensions.hpp>
//#endif

size_t vector_size = 100'000'000;

using namespace oneapi::tbb;

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
    srand(time(0));
    auto C = std::vector<float>(vector_size,1);
    auto A = std::vector<float>(vector_size,1);
    auto B = std::vector<float>(vector_size,1);

    std::thread thrA(ReadToIn, "TBB_A2.txt", &A),
        thrB(ReadToIn, "TBB_B2.txt", &B);
    thrA.join();
    thrB.join();

    //for (int i = 0; i < vector_size; i++)
    //{
    //    C2[i] = ((rand() % 200001) - 10000); //[-100'000:100'000]
    //    C3[i] = ((rand() % 200001) - 10000);
    //}

    auto begin = std::chrono::steady_clock::now();
    tbb::parallel_for(tbb::blocked_range<int>(0, C.size()),
                      [ & ](tbb::blocked_range<int> r)
                      {
                          for (int i = r.begin(); i < r.end(); ++i)
                          {
                              C[i] = A[i] + B[i];
                          }
                      });
    auto end = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "\nTime = " << diff_time.count() << "microseconds\n";

    auto itStart = C.begin();
    auto itEnd = C.rbegin();
    std::cout << "first elem = " << *itStart << "\nLast elem = " << *itEnd;

    return 0;
}
