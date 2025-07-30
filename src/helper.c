#include "helper.h"

Uint64 hash(char* s, Uint32 len)
{
    Uint64 h = 14695981039346656037ULL;
    for (Uint32 i = 0; i < len; i++)
    {
        h = (h ^ s[i]) * 1099511628211;
    }
    return h;
}

Uint64 hash_c_string(char* s)
{
    Uint64 h = 14695981039346656037ULL;
    while (*s)
    {
        h = (h ^ *s) * 1099511628211;
        s += 1;
    }
    return h;
}