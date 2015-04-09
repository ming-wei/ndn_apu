#include <CL/cl.hpp>

#include <cassert>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>

#include "trie.h"

namespace {
    constexpr int TIMES = 10;
}

void test_opencl()
{
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> devices;

    cl::Platform::get(&platforms);
    platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    printf("devices.size()=%lu\n", devices.size());

    cl_int err;
    cl::Context context(devices, NULL, NULL, NULL, &err);

    assert(err == CL_SUCCESS);
    context.getInfo(CL_CONTEXT_DEVICES, &devices);

    float x[100], y[100];
    for (int i = 0; i < 100; ++i) x[i] = 100 + i;

    cl::Buffer from(context, CL_MEM_USE_HOST_PTR, 100 * sizeof(float), x, &err);
    assert(err == CL_SUCCESS);
    cl::Buffer mid(context, CL_MEM_READ_WRITE, 100 * sizeof(float), NULL, &err);
    assert(err == CL_SUCCESS);
    cl::Buffer to(context, CL_MEM_USE_HOST_PTR, 100 * sizeof(float), y, &err);

    printf("%d\n", cl::copy(from));
    assert(CL_SUCCESS == cl::copy(x, x + 100, buffer));
    assert(CL_SUCCESS == cl::copy(buffer, y, y + 100));
    for (int i = 0; i < 10; ++i) {
        printf("%f %f\n", x[i], y[i]);
    }
}

int main()
{
    test_opencl();
    /*
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
    for (auto fname: traces) {
        std::cerr << "Querying " << fname << std::endl;
        std::ifstream is(fname);
        assert(is);
        int count_line = 0;
        while (is) {
            std::string line;
            std::getline(is, line);
            if (line.empty()) continue;
            int result = trie.query_port(line);
            (void)result;
            if (++count_line % 100 == 0) printf("%d\n", count_line);
        }
    }
    */
}

// vim: ft=cpp.doxygen
