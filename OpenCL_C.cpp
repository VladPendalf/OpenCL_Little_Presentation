////#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/opencl.hpp>
#endif
#define VECTOR_SIZE 100'000'000

//OpenCL kernel which is run for every work item created.
const char* saxpy_kernel =
"__kernel                                   \n"
"void saxpy_kernel(__global float *A,     \n"
"                  __global float *B,     \n"
"                  __global float *C)       \n"
"{                                          \n"
"    //Get the index of the work-item       \n"
"    int index = get_global_id(0);          \n"
"    C[index] = A[index] + B[index];        \n"
"}                                          \n";

int main(void)
{
   srand(time(0));
   int i;
   // Allocate space for vectors A, B and C
   float* A = (float*)malloc(sizeof(float) * VECTOR_SIZE);
   float* B = (float*)malloc(sizeof(float) * VECTOR_SIZE);
   float* C = (float*)malloc(sizeof(float) * VECTOR_SIZE);
   for (i = 0; i < VECTOR_SIZE; i++)
   {
       A[i] = ((rand() % 20001) - 10000); //[-100'000:100'000]
       B[i] = ((rand() % 20001) - 10000);
       C[i] = 0.;
   }

   // Get platform and device information
   cl_platform_id* platforms = NULL;
   cl_uint         num_platforms;

   cl_int          clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
   platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
   clStatus  = clGetPlatformIDs(num_platforms, platforms, NULL);

   cl_device_id* device_list = NULL;
   cl_uint       num_devices;
   clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
   device_list = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
   clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL);

   // Create one OpenCL context for each device in the platform
   cl_context context;
   context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus);

   // Create a command queue
   //cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);
   cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_list[0], 0, &clStatus);

   // Create memory buffers on the device for each vector
   cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);
   cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);
   cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);

   // Copy the Buffer A and B to the device
   clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);
   clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), B, 0, NULL, NULL);

   // Create a program from the kernel source
   cl_program program = clCreateProgramWithSource(context, 1, (const char**)&saxpy_kernel, NULL, &clStatus);

   // Build the program
   clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);

   // Create the OpenCL kernel
   cl_kernel kernel = clCreateKernel(program, "saxpy_kernel", &clStatus);

   // Set the arguments of the kernel
   //clStatus = clSetKernelArg(kernel, 0, sizeof(float), (void*)&alpha);
   clStatus = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&A_clmem);
   clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&B_clmem);
   clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&C_clmem);

   // Execute the OpenCL kernel on the list
   size_t global_size = VECTOR_SIZE; // Process the entire lists
   auto begin = std::chrono::steady_clock::now();
   clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
   auto end = std::chrono::steady_clock::now();

   // Read the cl memory C_clmem on device to the host variable C
   clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

   // Clean up and wait for all the comands to complete.
   clStatus = clFlush(command_queue);
   clStatus = clFinish(command_queue);

   auto diff_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
   std::cout << "\nTime = " << diff_time.count() << "microseconds\n";
   // Display the result to the screen
   for (i = 0; i < VECTOR_SIZE; i++)
       printf("%f + %f = %f\n", A[i], B[i], C[i]);

   // Finally release all OpenCL allocated objects and host buffers.
   clStatus = clReleaseKernel(kernel);
   clStatus = clReleaseProgram(program);
   clStatus = clReleaseMemObject(A_clmem);
   clStatus = clReleaseMemObject(B_clmem);
   clStatus = clReleaseMemObject(C_clmem);
   clStatus = clReleaseCommandQueue(command_queue);
   clStatus = clReleaseContext(context);
   //free(A);
   //free(B);
   free(C);
   free(platforms);
   free(device_list);
   return 0;
}
