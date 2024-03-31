#include <iostream>
#include <chrono>
#include <stdio.h>
#include <fstream>
#include <thread>
#include <stdlib.h>
#include <vector>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/opencl.hpp>
#endif
#define VECTOR_SIZE 100'000'000

std::string kernel_code =
"   void kernel simple_add(global const float* A,"
"                          global const float* B,"
"                          global float* C) {    "
"       int index = get_global_id(0);            "
"       C[index]=A[index]+B[index];              "
"}                                               ";

void ReadToOut(std::string outFilename,
               std::vector<float> data,
               std::chrono::microseconds diff_time)
{
    std::ofstream out(outFilename, std::ofstream::out);

    out << diff_time.count() << "microseconds\n";
    auto size = data.size();
    for (size_t i = 0; i < size; ++i)
        out << data[i] << "\n";
}

int main()
{
    srand(time(0));

    //get all platforms (drivers)
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    cl::Platform default_platform = all_platforms[0];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    cl::Device default_device = all_devices[0];
    std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

    //Imagine the Context as the runtime link to the our device and platform
    cl::Context context({ default_device });

    //Next we need to create the program which we want to execute on our device
    cl::Program::Sources sources;
    sources.push_back({ kernel_code.c_str(),kernel_code.length() });

    cl::Program program(context, sources);
    if (program.build({ default_device }) != CL_SUCCESS)
    {
        std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << "\n";
        exit(1);
    }

    // create buffers on the device
    cl::Buffer buffer_A(context, CL_MEM_READ_WRITE, sizeof(float) * VECTOR_SIZE);
    cl::Buffer buffer_B(context, CL_MEM_READ_WRITE, sizeof(float) * VECTOR_SIZE);
    cl::Buffer buffer_C(context, CL_MEM_READ_WRITE, sizeof(float) * VECTOR_SIZE);

    std::vector<float> A(VECTOR_SIZE, 0);
    std::vector<float> B(VECTOR_SIZE, 0);
    std::vector<float> C(VECTOR_SIZE, 0);
    for (int i = 0; i < VECTOR_SIZE; i++)
    {
        A[i] = ((rand() % 200001) - 10000); //[-100'000:100'000]
        B[i] = ((rand() % 200001) - 10000);
    }

    //create queue to which we will push commands for the device.
    cl::CommandQueue queue(context, default_device);

    //write arrays A and B to the device
    queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, sizeof(float) * VECTOR_SIZE, A.data());
    queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, sizeof(float) * VECTOR_SIZE, B.data());

    //run the kernel
    cl::Kernel kernel_add = cl::Kernel(program, "simple_add");
    //link args to kernel_funcctions
    kernel_add.setArg(0, buffer_A);
    kernel_add.setArg(1, buffer_B);
    kernel_add.setArg(2, buffer_C);
    auto begin = std::chrono::steady_clock::now();
    //args: объект_ядра, точка_начала, кол-во_work-item, какое_кол-во_work_item_давать_каждому_набору_данных
    queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(VECTOR_SIZE), cl::NullRange); //start work
    queue.finish(); //waiting while all cores finish work


    auto end = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "\nTime = " << diff_time.count() << "microseconds\n";

    //read result C from the device to array C
    queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, sizeof(float) * VECTOR_SIZE, C.data());

    //std::cout << " result: \n";
    //for (int i = 0; i < VECTOR_SIZE; i++)
    {
        //std::cout << C[i] << " ";
    }
    std::thread thrA(ReadToOut, "OpenCL_A2_00.txt", A, diff_time),
        thrB(ReadToOut, "OpenCL_B2.txt", B, diff_time),
        thrC(ReadToOut, "OpenCL_C2.txt", C, diff_time);

    thrA.join();
    thrB.join();
    thrC.join();
    std::cout << "You are end";
    return 0;
}