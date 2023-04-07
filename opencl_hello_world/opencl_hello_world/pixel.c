//
//  pixel.c
//  opencl_hello_world
//
//  Created by Stella on 4/7/23.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <OpenCL/opencl.h>

// work decomposition /16
// math xpos = xcos + ysin ypos = xsin + ycos
// rotate around the origin, no translation


/// #define MAX_SOURCE_SIZE (0x100000)
#define DEVICE_NAME_LEN 128
static char device_name[DEVICE_NAME_LEN];

static const char* path = "./img/img.bmp";

// kernel code: define new pos and bounds check
const char *KernelSource = "\n" \
"__kernel void rotation(                                                   \n" \
"   __global float* src_data;                                              \n" \
"   __global float* dest_data;                                             \n" \
"   int W, int H,                                                          \n" \
"   float sinTheta, float cosTheta)                                        \n" \
"{                                                                         \n" \
"   int ix = get_global_id(0);                                             \n" \
"   int iy = get_global_id(1);                                             \n" \
"   float xpos = ((float)ix) * cosTheta + ((float)iy) * sinTheta;          \n" \
"   float ypos = -1.0f * ((float)ix) * sinTheta + ((float)iy) * cosTheta;  \n" \
"   if(((int)xpos >= 0) && ((int)xpos < W) &&                              \n" \
"      ((int)ypos >= 0) && ((int)ypos < H) )                               \n" \
"       {                                                                  \n" \
"       dest_data[(int)ypos * W + (int)xpos] = src_data[iy*W + ix];        \n" \
"       }                                                                  \n" \
"}                                                                         \n" \
"\n";


int main ()
{
    cl_uint platformCount;
    cl_platform_id* platforms;
    cl_device_id device_id;
    cl_uint ret_num_devices;
    
    cl_int ret;
    int err;
    
    size_t global;
    size_t local;
    // size_t source_size;

    cl_context context = NULL;
    cl_command_queue command_queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    
    //device memory
    cl_mem input, output;
    
    float *hInput;
    float *hOutput;
    
    int rows;
    int cols;
    int i;
    
    // 45 degrees Theta 315 degrees
    float sinTheta = -0.70710678118;
    float cosTheta = 0.70710678118;
    
    // read bmp in floats
    hInput = (float *)readBmpFloat(path, &rows, &cols);
    printf("rows=%d, cols=%d\n", rows, cols);
    
    // output melloc: memory allocation
    hOutput = (float *)malloc(rows*cols*sizeof(float));
    for (i=0; i<rows*cols; i++)
        hOutput[i] = 1234.0;
    
    // get platform info
    clGetPlatformIDs(1, NULL, &platformCount);
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id*) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);
    
    // get device info
    // default gpu for mac
    int gpu = 1;
    ret = clGetDeviceIDs(platforms[0], gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, &ret_num_devices);
    ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, DEVICE_NAME_LEN,
                         device_name, NULL);
    printf("device name = &s \n", device_name);
    
    if (ret != CL_SUCCESS)
    {
        printf("Error: Failed to create a device group!\n");
        return EXIT_FAILURE;
    }
    
    // create context with the device &ret
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    if (!context)
    {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }
    
    // create command queue
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    if (!command_queue)
    {
        printf("Error: Failed to create a command / commands!\n");
        return EXIT_FAILURE;
    }
    
    
    // ??????????? source str, size 
    // create kernal program
    program = clCreateProgramWithSource(context, 1, (const char **) & KernelSource, NULL, &ret);
    if (!program)
    {
        printf("Error: Failed to create compute program!\n");
        return EXIT_FAILURE;
    }
    
    // build kernal program num_device: 1
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }
    
    // create kernel
    kernel = clCreateKernel(program, "rotation", &ret);
    if (!kernel || ret != CL_SUCCESS)
    {
        printf("Error: Failed to create compute kernel!\n");
        exit(1);
    }
    
    // create buffer
    input = clCreateBuffer(context,  CL_MEM_READ_ONLY,  rows*cols*sizeof(float), NULL, NULL);
    output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, rows*cols*sizeof(float), NULL, NULL);
    if (!input || !output)
    {
        printf("Error: Failed to allocate device memory!\n");
        exit(1);
    }
    

    // put data of the buffer into device memory / copy
    err = clEnqueueWriteBuffer(command_queue, input, CL_TRUE, 0, rows*cols*sizeof(float), hInput, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to write to source array!\n");
        exit(1);
    }
    
    
    // set kernel args
    err = 0;
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&output);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&input);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&cols);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&rows);
    err |= clSetKernelArg(kernel, 4, sizeof(cl_float), (void *)&sinTheta);
    err |= clSetKernelArg(kernel, 5, sizeof(cl_float), (void *)&cosTheta);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to set kernel arguments! %d\n", err);
        exit(1);
    }
    
    // get work group info
    err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to retrieve kernel work group info! %d\n", err);
        exit(1);
    }
    
    
    // execute the kernel 2 dimensions
    size_t globalws[2] = {cols, rows};
    size_t localws[2] = {8, 8};
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, &globalws, &localws, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error: Failed to execute kernel!\n");
        return EXIT_FAILURE;
    }
    
    clFinish(command_queue);
    
    
    // read/copy back to host from device to verify output
    err = clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, rows*cols*sizeof(float), (void *)hOutput, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to read output array! %d\n", err);
        exit(1);
    }
    printf("Output saved as img_rotated.bmp.\n");
    // write bmp into float
    writeBmpFloat(hOutput, "img_rotated.bmp", rows, cols, path);
    
    
    // Shutdown and cleanup
    free(hInput);
    free(hOutput);
    
    clReleaseMemObject(input);
    clReleaseMemObject(output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    
    
    return 0;
}
