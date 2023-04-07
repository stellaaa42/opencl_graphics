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

static const char* inputPath = "./img/img.bmp"

#define MAX_SOURCE_SIZE (0x100000)
#define DEVICE_NAME_LEN 128
static char dev_name[DEVICE_NAME_LEN]


// kernel code: define new pos and bounds check
const char *KernelSource = "\n" \
"__kernel void rotation(                                                   \n" \
"   __global float* src_data,                                              \n" \
"   __global float* dest_data,                                             \n" \
"   int W, int H,                                                          \n" \
"   float sinTheta, float cosTheta)                                        \n" \
"{                                                                         \n" \
"   int ix = get_global_id(0);                                             \n" \
"   int iy = get_global_id(1);                                             \n" \
"   float xpos = ((float)ix) * cosTheta + ((float)iy) * sinTheta,          \n" \
"   float ypos = -1.0 * ((float)ix) * sinTheta + ((float)iy) * cosTheta,  \n" \
"   if(((int)xpos >= 0) && ((int)xpos < W) &&                              \n" \
"      ((int)ypos >= 0) && ((int)ypos < H) )                               \n" \
"       {                                                                  \n" \
"       dest_data[(int)ypos * W + (int)xpos] = src_data[iy*W + ix]         \n" \
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
    
    size_t scource_size;
    char *source_str;
    
    cl_context context = NULL;
    cl_command_queue commands_queue = NULL;
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
    hInput = (float *)readBmpFloat(inputPath, &rows, &cols);
    printf("rows=%d, cols=%d\n", rows, cols);
    
    hOutput = (float *)
    
    
    
    
    // Shutdown and cleanup
}
