#include <SDL3/SDL.h>
#include "array.h"

/*
this is a quirk of C
the function definitions in array.h are marked 'inline'
however, we should also provide non-inlined versions of these functions 
so we provide these declarations here.
*/

bool _Array_Init(void** array, size_t element_size, size_t capacity);
bool _Array_Append_Slow(void** array, void* element);
bool _Array_Insert(void** array, size_t index, void* element);
void* _Array_Pop(void* array);
bool _Array_DeleteShift(void** array, size_t index);
bool _Array_DeleteSwap(void** array, size_t index);
bool _Array_Free(void** array);
