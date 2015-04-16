#include "common.h"
#include "trie.h"

int stt_query_port(const STT &stt, const std::string &line)
{
    int p = stt.root_id;
    int res = stt.ports[p];
    //printf("%s\n", line.c_str());
    //printf("cpu %d\n", p);
    //printf("stt %d\n", p);
    for (auto c: line) {
        const STTEntry &entry = stt.entries[p + c];
        //printf("query id %d\n", p + c);
        //printf("entry.vaild=%d, entry.c=%d c=%d\n", entry.valid, entry.c, c);
        if (entry.valid && entry.c == c) {
            p = entry.child_id;
            if (stt.ports[p] != -1) res = stt.ports[p];
            //printf("cpu %d\n", p);
            //printf("stt %d\n", p);
        } else {
            break;
        }
    }
    return res;
}
