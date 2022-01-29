#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <numeric>
#include <CL/opencl.h>

// #define CL3W_NO_CL_API_DEFINES
#include <cl3w.h>

#define CL_CHECK(err) do {                                     \
    auto e = (err);                                            \
    if(e != CL_SUCCESS) {                                      \
        std::cerr << "OpenCL Error Code: " << e << std::endl;  \
        exit(-1);                                              \
    }                                                          \
} while(0)

const char kernelSource[] =
"__kernel                                                                    "
"void vectorAdd(global float* C, global float* A, global float* B, int N) {  "
"    int i = get_global_id(0);                                               "
"    if (i >= N) return;                                                     "
"    C[i] = A[i] + B[i];                                                     "
"}                                                                           ";

int main() {
    cl3wInit();

    cl_device_id device;
    cl_context context;
    cl_command_queue commandQueue;
    cl_kernel vectorAddKernel;
    cl_program program;
    cl_int err;
    cl_mem bufferA;
    cl_mem bufferB;
    cl_mem bufferC;

    cl_uint numPlatforms = 0;
    CL_CHECK(clGetPlatformIDs(0, nullptr, &numPlatforms));
    if (numPlatforms == 0) {
        std::cerr << "numPlatforms == 0" << std::endl;
        return 1;
    }

    cl_platform_id platform;
    CL_CHECK(clGetPlatformIDs(1, &platform, nullptr));

    size_t platformNameLen = 0;
    CL_CHECK(clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, nullptr, &platformNameLen));
    std::string platformName(platformNameLen, '\0');
    CL_CHECK(clGetPlatformInfo(platform, CL_PLATFORM_NAME, platformNameLen, &platformName[0], nullptr));
    std::cout << "OpenCL Platform: " << platformName << std::endl;;

    CL_CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, nullptr));
    size_t deviceNameBufferSize = 0;
    CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &deviceNameBufferSize));
    std::string deviceName(deviceNameBufferSize, '\0');
    CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_NAME, deviceNameBufferSize, &deviceName[0], nullptr));
    std::cout <<"OpenCL Device: " << deviceName << std::endl;

    err = CL_SUCCESS;
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    CL_CHECK(err);

    size_t kernelSourceLength = sizeof(kernelSource);
    const char* kernelSourceArray = { kernelSource };
    program = clCreateProgramWithSource(context, 1, &kernelSourceArray, &kernelSourceLength, &err);
    CL_CHECK(err);

    if (clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr) != CL_SUCCESS)
    {
        size_t programBuildLogBufferSize = 0;
        CL_CHECK(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &programBuildLogBufferSize));
        std::string programBuildLog(programBuildLogBufferSize, '\0');
        CL_CHECK(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, programBuildLogBufferSize, &programBuildLog[0], nullptr));
        std::clog << programBuildLog << std::endl;
        return 1;
    }

    vectorAddKernel = clCreateKernel(program, "vectorAdd", &err);
    CL_CHECK(err);

    commandQueue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    CL_CHECK(err);

    size_t N = 10 * 1000 * 1000 / sizeof(float); // 10M of floats
    std::vector<float> hostA, hostB, hostC;

    hostA.resize(N);
    hostB.resize(N);
    hostC.resize(N);

    std::iota(std::begin(hostA), std::end(hostA), 0);
    std::iota(std::begin(hostB), std::end(hostB), 0);


    bufferA = clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), nullptr, &err);
    CL_CHECK(err);
    bufferB = clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), nullptr, &err);
    CL_CHECK(err);
    bufferC = clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), nullptr, &err);
    CL_CHECK(err);

    CL_CHECK(clEnqueueWriteBuffer(commandQueue, bufferA, CL_FALSE, 0, N * sizeof(float), hostA.data(), 0, nullptr, nullptr));
    CL_CHECK(clEnqueueWriteBuffer(commandQueue, bufferB, CL_FALSE, 0, N * sizeof(float), hostB.data(), 0, nullptr, nullptr));
    CL_CHECK(clSetKernelArg(vectorAddKernel, 0, sizeof(cl_mem), &bufferC));
    CL_CHECK(clSetKernelArg(vectorAddKernel, 1, sizeof(cl_mem), &bufferA));
    CL_CHECK(clSetKernelArg(vectorAddKernel, 2, sizeof(cl_mem), &bufferB));
    CL_CHECK(clSetKernelArg(vectorAddKernel, 3, sizeof(cl_int), &N));
    CL_CHECK(clEnqueueNDRangeKernel(commandQueue, vectorAddKernel, 1, nullptr, &N, nullptr, 0, nullptr, nullptr));
    CL_CHECK(clEnqueueReadBuffer(commandQueue, bufferC, CL_FALSE, 0, N * sizeof(float), hostC.data(), 0, nullptr, nullptr));
    CL_CHECK(clFinish(commandQueue));

    for (int i = 0; i < N; ++i) {
        if(hostC[i] != hostA[i] + hostA[i]) {
            std::cerr << hostC[i] << " != " <<  hostA[i] << " + " << hostB[i] << std::endl;
            return -1;
        }
    }
    std::cout << "All value matched!" << std::endl;
    return 0;
}
