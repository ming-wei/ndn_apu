__kernel void double_kernel(__global int *data, __global int *flag)
{
    const int FLAG_SIZE = 65536;
    int i = get_global_id(0);
    int count = 0;
    int tmp = 0;
    int xflag = 0;
    while (1) {
        for (int i = 0; i < FLAG_SIZE; i += 1) xflag |= flag[i];
        if (xflag != 0) break;
    }
    data[i] = xflag;
}

// vim: ft=cpp
