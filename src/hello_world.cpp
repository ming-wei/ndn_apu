#include <CL/cl.hpp>

#include <cassert>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>

#include "trie.h"
#include "common.h"


namespace {
    constexpr int TIMES = 1;
}

const char *kernel_sources = "\n" \
"__kernel void square(__global float *input, __global float *output, \n" \
"const int count) \n" \
"{ \n" \
"  int i = get_global_id(0); \n" \
"  if (i < count) output[i] = input[i] * input[i]; \n" \
"} \n";

void test_opencl()
{
    int err;
    constexpr int SIZE = 100;
    float data[SIZE], results[SIZE];

    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_context context;
    cl_command_queue commands;
    cl_program program;
    cl_kernel kernel;

    cl_mem input, output;

    // filldata
    for (int i = 0; i < SIZE; ++i) data[i] = (float)rand() / RAND_MAX;

    err = clGetPlatformIDs(1, &platform_id, NULL);
    printf("%s\n", get_error_str(err));
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, NULL);

    printf("%s\n", get_error_str(err));
    CK(err);
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    CK(err);

    commands = clCreateCommandQueue(context, device_id, 0, &err);
    CK(err);

    program = clCreateProgramWithSource(context, 1,(const char **) &kernel_sources, NULL, &err);
    CK(err);

    err = clBuildProgram(program, 0, 0, 0, 0, 0);
    if (err != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }
    input = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * SIZE, NULL, &err);
    CK(err);
    output = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * SIZE, NULL, &err);
    CK(clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, sizeof(float) * SIZE,
            data, 0, NULL, NULL));

    kernel = clCreateKernel(program, "square", &err);
    CK(err);
    CK(clSetKernelArg(kernel, 0, sizeof(cl_mem), &input));
    CK(clSetKernelArg(kernel, 1, sizeof(cl_mem), &output));
    CK(clSetKernelArg(kernel, 2, sizeof(int), &SIZE));

    size_t local;
    CK(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
                sizeof(local), &local, NULL));
    size_t global = SIZE;
    CK(clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL));
    CK(clFinish(commands));
    CK(clEnqueueReadBuffer(commands, output, CL_TRUE, 0, sizeof(float) * SIZE, results, 0,
            NULL, NULL));
    for (int i = 0; i < SIZE; ++i) printf("%f %f\n", data[i], results[i]);
}

void query_batched(Trie &trie, STT &stt, STT_GPU &stt_gpu)
{
    //
    /*
    printf("n=%d\n", stt_gpu.n);
    printf("barrier_data=");
    for (size_t i = 0; i <= stt_gpu.n; ++i) printf("%d ", stt_gpu.barrier_data[i]);
    printf("\n");
    printf("str_data=");
    for (size_t i = 0; i <= stt_gpu.barrier_data[stt_gpu.n]; ++i) printf("%c", stt_gpu.str_data[i]);
    printf("\n");
    */
    //
    std::vector<int> res1, res2;
    size_t n = stt_gpu.n;
    res1.resize(n);
    res2.resize(n);
    for (size_t i = 0; i < n; ++i)  {
        res1[i] = stt_query_port(stt, std::string(
                    stt_gpu.str_data + stt_gpu.barrier_data[i],
                    stt_gpu.str_data + stt_gpu.barrier_data[i+1]));
    }
    stt_gpu.query();
    for (size_t i = 0; i < n; ++i) 
        res2[i] = stt_gpu.output_data[i];
    for (size_t i = 0; i < n; ++i) {
        assert(res1[i] == res2[i]);
    }
    printf("query batch finished\n");
}

int main()
{
    // test_opencl();
    /// constructing trie
    std::vector<std::string> fibs;
    for (int i = 1; i <= TIMES; ++i) {
        std::ostringstream os;
        os << "/mnt/sdb1/fibs_and_traces/3M_fibs/3_";
        os << i;
        os << ".txt";
        fibs.push_back(os.str());
    }
    Trie trie(fibs);
    STT stt;
    trie.construct_stt(stt);
    
    STT_GPU stt_gpu;
    stt_gpu.init(stt);
    /// do queries
    std::vector<std::string> traces;
    for (int i = 1; i <= TIMES; ++i) {
        std::ostringstream os;
        os << "/mnt/sdb1/fibs_and_traces/3M_trace/a_3_";
        os << i;
        os << ".trace";
        traces.push_back(os.str());
    }
    for (int i = 1; i <= TIMES; ++i) {
        std::ostringstream os;
        os << "/mnt/sdb1/fibs_and_traces/3M_trace/w_3_";
        os << i;
        os << ".trace";
        traces.push_back(os.str());
    }
    // initialize stt_gpu
    stt_gpu.n = 0;
    int offset = 0;
    for (auto fname: traces) {
        std::cerr << "Querying " << fname << std::endl;
        std::ifstream is(fname);
        assert(is);
        while (is) {
            if (!is.getline(stt_gpu.str_data + offset, QUERY_LEN)) continue;
            stt_gpu.barrier_data[stt_gpu.n] = offset;
            ++stt_gpu.n;
            offset += strlen(stt_gpu.str_data + offset);
            if (stt_gpu.n == BATCH_SIZE) {
                stt_gpu.barrier_data[stt_gpu.n] = offset;
                query_batched(trie, stt, stt_gpu);
                stt_gpu.n = 0;
                offset = 0;
            }
        }
    }
}

// vim: ft=cpp.doxygen
