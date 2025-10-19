#ifndef HASH_H
#define HASH_H

#include <SDL3/SDL.h>

Uint64 hash(char* s, Uint32 len);
Uint64 hash_c_string(char* s);

#endif // HASH_H