#include "common.h"
#include <cassert>
#include <memory>
#include <fstream>
#include <cstring>
#include <thread>
#include <unistd.h>

const int FLAG_SIZE = 65536;

cl_program create_program_from_file(cl_context context, const char *filename, 
        cl_int *err)
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

void flag_modifier(cl_command_queue commands, void *flag)
{
    usleep(1000);
    printf("before map\n");
    CK(clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_WRITE, flag,
            sizeof(int), 0, NULL, NULL));
    printf("before modify\n");

    for (int i = 0; i < FLAG_SIZE; ++i)
        ((int *)flag)[i] = 1;
    printf("after modify\n");

    CK(clEnqueueSVMUnmap(commands, flag, 0, NULL, NULL));
    CK(clFinish(commands));
    printf("after unmap\n");

}

void test_kernel_daemon()
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

    cl_device_svm_capabilities caps;

    err = clGetDeviceInfo(
            device,
            CL_DEVICE_SVM_CAPABILITIES,
            sizeof(cl_device_svm_capabilities),
            &caps,
            0
            );
    CK(err);

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CK(err);

    cl_command_queue commands = clCreateCommandQueueWithProperties(
            context, device, 0, &err);
    cl_command_queue commands2 = clCreateCommandQueueWithProperties(
            context, device, 0, &err);
    CK(err);

    int N = 128;
    int *data = (int *)clSVMAlloc(context, CL_MEM_READ_WRITE,
            sizeof(int) * N, 0);
    int *flag = (int *)clSVMAlloc(context, CL_MEM_READ_WRITE,
            sizeof(int) * FLAG_SIZE, 0);
    assert(data);
    assert(flag);

    CK(clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_WRITE, data,
            sizeof(int) * N, 0, NULL, NULL));
    CK(clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_WRITE, flag,
            sizeof(int) * FLAG_SIZE, 0, NULL, NULL));

    for (int i = 0; i < N; ++i) data[i] = i;
    for (int i = 0; i < FLAG_SIZE; ++i) flag[i] = 0;

    CK(clEnqueueSVMUnmap(commands, data, 0, NULL, NULL));
    CK(clEnqueueSVMUnmap(commands, flag, 0, NULL, NULL));

    cl_program program = create_program_from_file(context,
            "src/double_kernel.cl", &err);
    CK(err);

    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", 0, 0);
    if (err != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }

    cl_kernel kernel = clCreateKernel(program, "double_kernel", &err);
    CK(err);

    CK(clSetKernelArgSVMPointer(kernel, 0, (void *)data));
    CK(clSetKernelArgSVMPointer(kernel, 1, (void *)flag));
    
    size_t global_work_size = N, local_work_size = N;

    CK(clFinish(commands));
    std::thread T(flag_modifier, commands2, flag);
    printf("before kernel\n");
    CK(clEnqueueNDRangeKernel(commands, kernel, 1, NULL, 
                &global_work_size, &local_work_size, 0, NULL, NULL));
    CK(clFinish(commands));
    printf("after kernel\n");
    usleep(1e6);
    T.join();

    CK(clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_READ, data,
            sizeof(int) * N, 0, NULL, NULL));
    for (int i = 0; i < 100; ++i) {
        printf("%d ", data[i]);
    }
    CK(clEnqueueSVMUnmap(commands, data, 0, NULL, NULL));
    
    exit(0);
}
