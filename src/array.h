#ifndef ARRAY_H
#define ARRAY_H
#include <SDL3/SDL.h>

/*

typedef struct Array_X
{
    X* arr;
    Uint32 len;
    Uint32 cap;
} Array_X;

*/

#define Array_Initialize(ARRAY) \
    do \
    { \
        Uint32 new_cap = 4096 / sizeof((ARRAY)->arr[0]);\
        new_cap = new_cap < 8 ? 8 : new_cap; \
        void* temp_ptr = SDL_realloc((ARRAY)->arr, new_cap * sizeof((ARRAY)->arr[0])); \
        if (temp_ptr == NULL) \
        { \
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize Array memory!"); \
            SDL_memset((ARRAY), 0, sizeof(*(ARRAY))); \
        } \
        else \
        { \
            (ARRAY)->arr = temp_ptr; \
            (ARRAY)->cap = new_cap; \
            (ARRAY)->len = 0; \
        } \
    } while (0)

#define Array_Append(ARRAY, ELEMENT) \
    do \
    { \
        if ((ARRAY)->len >= (ARRAY)->cap) \
        { \
            Uint32 new_cap = (ARRAY)->cap * 2; \
            void* temp_ptr = SDL_realloc((ARRAY)->arr, new_cap * sizeof((ARRAY)->arr[0])); \
            if (temp_ptr == NULL) \
            { \
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reallocate array memory!"); \
            } \
            else \
            { \
                (ARRAY)->arr = temp_ptr; \
                (ARRAY)->cap = new_cap; \
            } \
        } \
        if ((ARRAY)->len < (ARRAY)->cap) \
        { \
            (ARRAY)->arr[(ARRAY)->len] = ELEMENT; \
            (ARRAY)->len++; \
        } \
    } while (0)


#endif // ARRAY_H