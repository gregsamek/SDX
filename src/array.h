#ifndef ARRAY_H
#define ARRAY_H

bool _Array_Init(void** array, size_t element_size, size_t capacity);
bool _Array_Append_Slow(void** array, void* element);
bool _Array_Insert(void** array, size_t index, void* element);
void* _Array_Pop(void* array);
bool _Array_DeleteShift(void** array, size_t index);
bool _Array_DeleteSwap(void** array, size_t index);
bool _Array_Free(void** array);

/**********************************************

// Example Usage

Vec3 Array arr = Array_Init(Vec3, 0);
Vec3 value = { 1, 2, 3 };
Array_Append(arr, value);
foreach(v, arr)
{
    printf("item = %f %f %f\n", v.x, v.y, v.z);
}
Array_Free(arr);

**********************************************/

typedef struct
{
    size_t len;
    size_t capacity;
    size_t element_size;
    void* allocator;
} _Array_Header;

#define Array *
#define Array_Header(_A_) ((_Array_Header*)((char*)(_A_) - sizeof(_Array_Header)))
#define Array_Len(_A_) Array_Header(_A_)->len
#define Array_Capacity(_A_) Array_Header(_A_)->capacity
#define Array_ElementSize(_A_) Array_Header(_A_)->element_size
#define Array_Allocator(_A_) Array_Header(_A_)->allocator

#define Array_Init(ARRAY, CAPACITY) _Array_Init((void**)&(ARRAY), sizeof(*(ARRAY)), CAPACITY)
#define Array_Append_Fast(ARRAY, ELEMENT) ((ARRAY)[Array_Header(ARRAY)->len++] = (ELEMENT), true)
#define Array_Append(ARRAY, ELEMENT) (Array_Len(ARRAY) >= Array_Capacity(ARRAY) ? _Array_Append_Slow((void**)&(ARRAY), &(ELEMENT)) : Array_Append_Fast(ARRAY, ELEMENT))
#define Array_Insert(ARRAY, INDEX, ELEMENT) _Array_Insert((void**)&(ARRAY), INDEX, &(ELEMENT))
#define Array_Pop(ARRAY) _Array_Pop(ARRAY)
#define Array_DeleteShift(ARRAY, INDEX) _Array_DeleteShift((void**)&(ARRAY), INDEX)
#define Array_DeleteSwap(ARRAY, INDEX) _Array_DeleteSwap((void**)&(ARRAY), INDEX)
#define Array_Free(ARRAY) _Array_Free((void**)&(ARRAY))

#define concat(a, b) a##b
#define macro_var(name) concat(name, __LINE__)
#define foreach(item, ARRAY) \
    for (__typeof__(ARRAY) macro_var(_arr_) = (ARRAY); macro_var(_arr_); macro_var(_arr_) = 0) \
    for (size_t macro_var(_i_) = 0, macro_var(_len_) = Array_Len(macro_var(_arr_)); macro_var(_i_) < macro_var(_len_); ++macro_var(_i_)) \
    for (__typeof__(*ARRAY) item = macro_var(_arr_)[macro_var(_i_)], *macro_var(_keep_) = &item; macro_var(_keep_); macro_var(_keep_) = 0)

#endif // ARRAY_H