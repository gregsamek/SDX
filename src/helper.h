#ifndef HELPER_H
#define HELPER_H

#include <SDL3/SDL.h>

Uint64 hash(char* s, Uint32 len);
Uint64 hash_c_string(char* s);

// #define offsetof(_STRUCT, _MEMBER) ((size_t)((char *)&((_STRUCT *)0)->_MEMBER - (char *)0))

#endif // HELPER_H