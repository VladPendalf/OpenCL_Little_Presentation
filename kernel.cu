
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

#define VECTOR_SIZE 100'000'000

std::chrono::steady_clock::time_point begin, end;
std::chrono::microseconds diff_time;

cudaError_t addWithCuda(float* c, float* a, float* b, unsigned int size);

__global__ void addKernel(float *c, float *a, float *b)
{
    //int i = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    c[i] = a[i] + b[i];
}

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

void ReadToOut(std::string outFilename,
               std::vector<float> data)
{
    std::ofstream out(outFilename, std::ofstream::out);

    auto size = data.size();
    for (size_t i = 0; i < size; ++i)
        out << data[i] << "\n";
}

int main()
{
    srand(time(0));
    std::vector<float> a(VECTOR_SIZE,0);
    std::vector<float> b(VECTOR_SIZE,0);
    std::vector<float> c(VECTOR_SIZE,0);

    std::thread thrA(ReadToIn, "Cuda_A2.txt", &a),
        thrB(ReadToIn, "Cuda_B2.txt", &b);
    thrA.join();
    thrB.join();


    //for (int i = 0; i < VECTOR_SIZE; i++)
    //{
    //    a[i] = ((rand() % 200001) - 10000); //[-100'000:100'000]
    //    b[i] = ((rand() % 200001) - 10000);
    //}

    // Add vectors in parallel.
    cudaError_t cudaStatus = addWithCuda(c.data(), a.data(), b.data(), VECTOR_SIZE);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addWithCuda failed!");
        return 1;
    }

    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }

    std::thread thrC(ReadToOut, "Cuda_C2.txt", c);
    thrC.join();
    std::cout << "\nOutput end\n";

    return 0;
}

// Helper function for using CUDA to add vectors in parallel.
cudaError_t addWithCuda(float* c, float* a, float* b, unsigned int size)
{
    float *dev_a = 0;
    float *dev_b = 0;
    float *dev_c = 0;
    cudaError_t cudaStatus;

    // Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        goto Error;
    }

    // Allocate GPU buffers for three vectors (two input, one output)    .
    cudaStatus = cudaMalloc((void**)&dev_c, size * sizeof(float));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_a, size * sizeof(float));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_b, size * sizeof(float));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

    // Copy input vectors from host memory to GPU buffers.
    cudaStatus = cudaMemcpy(dev_a, a, size * sizeof(float), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }

    cudaStatus = cudaMemcpy(dev_b, b, size * sizeof(float), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }

    // Launch a kernel on the GPU with one thread for each element.
    begin = std::chrono::steady_clock::now();
    //<<<AllSizeData/(2*count_of_cores_in_your_GPU), (2*count_of_cores_in_your_GPU)>>>
    addKernel<<<VECTOR_SIZE/1000, 1000>>>(dev_c, dev_a, dev_b);

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
        goto Error;
    }
    
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
        goto Error;
    }
    end = std::chrono::steady_clock::now();
    diff_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "\nTime = " << diff_time.count() << "microseconds\n";

    // Copy output vector from GPU buffer to host memory.
    cudaStatus = cudaMemcpy(c, dev_c, size * sizeof(float), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }

Error:
    cudaFree(dev_c);
    cudaFree(dev_a);
    cudaFree(dev_b);
    
    return cudaStatus;
}
