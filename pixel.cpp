//
//  pixel.cpp
//  opencl_hello_world
//
//  Created by Stella on 4/12/23.
//
// Youtube: https://youtu.be/mW2i8rLUqG8


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
// open computing language
// a parallel computing API framework
#include <OpenCL/opencl.h>

// computer vision and image processing
// brew install, link lib /usr/local/Cellar/opencv/4.7.0_2
// header search paths /usr/local/include/opencv4
#include <opencv2/opencv.hpp>

// Create OpenCL kernel
// math: xt = (x - xc) * cos_angle - (y - yc) * sin_angle + xc
// (xc, yc) pivot point, translate relative to the origin point
//  transform output[yt * width + xt] = input[y * width + x]
const char* kernel_source = "__kernel void rotation(__global const float* input, __global float* output, const int width, const int height, const float angle)"
                            "{"
                            "    const int x = get_global_id(0);"
                            "    const int y = get_global_id(1);"
                            "    const float cos_angle = cos(angle);"
                            "    const float sin_angle = sin(angle);"
                            "    const int xc = width / 2;"
                            "    const int yc = height / 2;"
                            "    const int xt = (x - xc) * cos_angle - (y - yc) * sin_angle + xc;"
                            "    const int yt = (x - xc) * sin_angle + (y - yc) * cos_angle + yc;"
                            "    if (xt >= 0 && xt < width && yt >= 0 && yt < height)"
                            "        output[yt * width + xt] = input[y * width + x];"
                            "}";



void load_image(const char* filename, int* width, int* height, float** data)
{
    cv::Mat image = cv::imread(filename, cv::IMREAD_GRAYSCALE);
    *width = image.cols;
    *height = image.rows;
    // load into memory
    *data = (float*) malloc(*width * *height * sizeof(float));
    for (int y = 0; y < *height; y++)
    {
        for (int x = 0; x < *width; x++)
        {
            // (y,x) computer into 2D float array
            // /255.0f nomalize in between 0 and 1
            // *data *width: to dereference and access the real data
            (*data)[y * *width + x] = (float) image.at<uint8_t>(y, x) / 255.0f;
            /*
             // loop thru the rgb pixel, store into float arr
            for (int i = 0; i < rows * cols ; i++) {
                float r = (float)data[i * 3 + 2];
                float g = (float)data[i * 3 + 1];
                float b = (float)data[i * 3];
                image[i] = (r + g + b) / 3.0f;
                // printf("image i %f\n", image[i]);
            }
            */
        }
    }
}

void save_image(const char* filename, int width, int height, float* data)
{
    // preprocessor macro defined single-channel 32-bit floating-point image
    cv::Mat image(height, width, CV_32FC1, data);
    cv::imwrite(filename, image * 255.0f);
}

void check_error(cl_int err, const char* message)
{
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "%s (error code %d)\n", message, err);
        exit(EXIT_FAILURE);
    }
}

int main()
{
    // const char* filename = "./image.jpg";
    // absolute path
    // /Users/Stella/project_x/opencl/opencl_graphics/opencl_hello_world/opencl_hello_world
    const char* filename = "/Users/Stella/project_x/opencl/opencl_graphics/opencl_hello_world/opencl_hello_world/image.jpg";
    // os.path.exist
    if (access(filename, F_OK) == 0) {
            // File exists
            std::cout << "File exists" << std::endl;
        } else {
            std::cout << "File does not exist" << std::endl;
        }
    
    const float angle = 45.0f;

    // Load image into memory
    int width, height;
    float* input_data;
    load_image(filename, &width, &height, &input_data);

    // Initialize OpenCL devices, context and steps
    # define DEVICE_NAME_LEN 128
    static char dev_name[DEVICE_NAME_LEN];
    
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem input_buffer, output_buffer;
    cl_int err;
    cl_int ret;

    // get platform, device
    err = clGetPlatformIDs(1, &platform, NULL);
    check_error(err, "Error getting OpenCL platform ID");
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    std::cout << "device " << device << std::endl;
    check_error(err, "Error getting OpenCL device ID");
    ret = clGetDeviceInfo(device, CL_DEVICE_NAME, DEVICE_NAME_LEN, dev_name, NULL);
    // print the device engine GPU name
    std::cout << "device name == " << dev_name << std::endl;
    check_error(ret, "Error getting OpenCL device info");

    
    // context to connect to that device, gpu
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    check_error(err, "Error creating OpenCL context");
    // create command queue
    command_queue = clCreateCommandQueue(context, device, 0, &err);
    check_error(err, "Error creating OpenCL command queue");

    // create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    check_error(err, "Error creating OpenCL program");

    // build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t log_size;
        char* log;

        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        log = (char*) malloc(log_size + 1);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        log[log_size] = '\0';
        fprintf(stderr, "Error building OpenCL program: %s\n", log);
        free(log);
        exit(EXIT_FAILURE);
    }

    // create kernel
    kernel = clCreateKernel(program, "rotation", &err);
    check_error(err, "Error creating OpenCL kernel");

    // Create OpenCL memory buffers
    input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, width * height * sizeof(float), input_data, &err);
    check_error(err, "Error creating OpenCL input buffer");
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, width * height * sizeof(float), NULL, &err);
    check_error(err, "Error creating OpenCL output buffer");

    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);
    err |= clSetKernelArg(kernel, 2, sizeof(int), &width);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &height);
    err |= clSetKernelArg(kernel, 4, sizeof(float), &angle);
    check_error(err, "Error setting OpenCL kernel arguments");

    // Execute kernel
    // unsigned int cast
    // size_t global_size[2] = { width, height };
    size_t global_size[2];
    // cast width to size_t first
    global_size[0] = static_cast<size_t>(width);
    global_size[1] = static_cast<size_t>(height);
    err = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);
    check_error(err, "Error executing OpenCL kernel");
    
    // Wait for the commands to get serviced before reading back results
    // asynchronous, wait for everything to finish
    clFinish(command_queue);

    // Read output back from device
    float* output_data = (float*) malloc(width * height * sizeof(float));
    err = clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0, width * height * sizeof(float), output_data, 0, NULL, NULL);
    check_error(err, "Error reading OpenCL output buffer");

    
    // Save rotated image to file
    save_image("/Users/Stella/project_x/opencl/opencl_graphics/opencl_hello_world/opencl_hello_world/rotated_image.jpg", width, height, output_data);

    // Clean up buffers and memories
    clReleaseMemObject(input_buffer);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    free(input_data);
    free(output_data);

    return 0;
}



