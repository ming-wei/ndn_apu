#include "common.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <cassert>

void STT_GPU::begin_load()
{
    CK(clEnqueueSVMMap(commands, true, CL_MAP_WRITE,
                barrier_data, sizeof(int) * (BATCH_SIZE + 1),
                0, 0, 0));
    CK(clEnqueueSVMMap(commands, true, CL_MAP_WRITE,
                str_data, sizeof(char) * BATCH_SIZE * QUERY_LEN,
                0, 0, 0));
}

void STT_GPU::end_load()
{
    CK(clEnqueueSVMUnmap(commands, barrier_data, 0, 0, 0));
    CK(clEnqueueSVMUnmap(commands, str_data, 0, 0, 0));
}


void STT_GPU::begin_save()
{
    CK(clEnqueueSVMMap(commands, true, CL_MAP_READ,
                output_data, sizeof(int) * BATCH_SIZE,
                0, 0, 0));
}

void STT_GPU::end_save()
{
    CK(clEnqueueSVMUnmap(commands, output_data, 0, 0, 0));
}
void STT_GPU::init(const STT &stt_src)
{
    cl_int err;
    cl_uint num_platforms, num_devices;
    cl_platform_id platform;
    cl_device_id device;

    err = clGetPlatformIDs(1, &platform, &num_platforms);
    CK(err);
    assert(num_platforms == 1);

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, 
            &num_devices);
    CK(err);
    assert(num_devices == 1);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CK(err);

    commands = clCreateCommandQueueWithProperties(context, device, 0, &err);
    CK(err);

    size_t n = stt_src.entries.size();
    stt = clCreateBuffer(context, 
            CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 
            sizeof(STTEntry) * n, NULL, &err);
    CK(err);

    ports = clCreateBuffer(context, 
            CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD,
            sizeof(int) * n, NULL, &err);

    err = clEnqueueWriteBuffer(commands, stt, CL_TRUE, 0, 
            sizeof(STTEntry) * n, stt_src.entries.data(), 0, NULL, NULL);
    CK(err);

    err = clEnqueueWriteBuffer(commands, ports, CL_TRUE, 0,
            sizeof(int) * n, stt_src.ports.data(), 0, NULL, NULL);
    CK(err);

    barrier_data = (int *)clSVMAlloc(context, CL_MEM_READ_ONLY, sizeof(int) *
            (BATCH_SIZE + 1), 0);
    CK(err);

    str_data = (char *)clSVMAlloc(context, CL_MEM_READ_ONLY, sizeof(char) * 
            BATCH_SIZE * QUERY_LEN, 0);
    CK(err);

    output_data = (int *)clSVMAlloc(context, CL_MEM_WRITE_ONLY, sizeof(int) *
            BATCH_SIZE, 0);
    CK(err);
    CK(clFinish(commands));
    /*
    for (int p = 0; p < 100; ++p) {
        const STTEntry entry = stt_src.entries[p];
        //printf("gpu %d entry.vaild=%d, entry.c=%d\n", p, entry.valid, entry.c);
    }
    */

    program = create_program_from_file("src/kernel.cl", &err);
    CK(err);
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", 0, 0);
    if (err != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }

    kernel = clCreateKernel(program, "query_kernel", &err);
    CK(err);
    /*
    CK(clSetKernelArg(kernel, 0, sizeof(cl_mem), &barrier));
    CK(clSetKernelArg(kernel, 1, sizeof(cl_mem), &str));
    CK(clSetKernelArg(kernel, 2, sizeof(cl_mem), &output));
    */
    CK(clSetKernelArgSVMPointer(kernel, 0, barrier_data));
    CK(clSetKernelArgSVMPointer(kernel, 1, str_data));
    CK(clSetKernelArgSVMPointer(kernel, 2, output_data));
    CK(clSetKernelArg(kernel, 3, sizeof(cl_mem), &stt));
    CK(clSetKernelArg(kernel, 4, sizeof(cl_mem), &ports));
}


cl_program STT_GPU::create_program_from_file(const char *filename, cl_int *err)
{
    std::ifstream t(filename);
    assert(t);
    std::string newline;
    std::string source;
    while (std::getline(t, newline)) {
        source.append(newline);
        source.append("\n");
    }
    char *source_c = new char[16384];
    strcpy(source_c, source.c_str());
    cl_program res =  clCreateProgramWithSource(context, 1, 
            (const char **)&source_c, NULL, err);
    //printf("program:\n%s\n", source_c);
    return res;
}

void STT_GPU::query()
{
    size_t global_work_size = 128, local_work_size = 128;
    CK(clEnqueueNDRangeKernel(commands, kernel, 1, NULL, 
                &global_work_size, &local_work_size, 0,
                NULL, NULL));
    CK(clFinish(commands));
}

void STT_GPU::fini()
{
    CK(clReleaseMemObject(stt));
    CK(clReleaseMemObject(ports));
}
