typedef struct STTEntry_t {
    int c;
    int valid;
    int child_id;
    int padding;
} STTEntry;

__kernel void query_kernel(
        __global int *barrier,
        __global char *str,
        __global int *output,
        __global STTEntry *stt,
        __global int *ports)
{
    int i = get_global_id(0);
    int p = 0, res = ports[p];
    /*
    for (int p = 0; p < 100; ++p) {
        const STTEntry entry = stt[p];
        printf("gpu %d entry.vaild=%d, entry.c=%d\n", p, entry.valid, entry.c);
    }
    */
    //for (int i = 0; i < 100; ++i) printf("gpu %d %d\n", i, ports[i]);
    int start = barrier[i], end = barrier[i + 1];
    for (int j = start; j < end; ++j) {
        char c = str[j];
        STTEntry entry = stt[p+c];
        //printf("gpu entry.vaild=%d, entry.c=%d c=%d\n", entry.valid, entry.c, c);
        if (entry.valid && entry.c == c) {
            // ok
            p = entry.child_id;
            if (ports[p] != -1) res = ports[p];
        } else {
            break;
        }
    }
    output[i] = res;
}


// vim: ft=cpp
