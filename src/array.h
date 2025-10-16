#ifndef ARRAY_H
#define ARRAY_H



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

inline bool _Array_Init(void** array, size_t element_size, size_t capacity)
{
    if (capacity == 0) capacity = (4096 - sizeof(_Array_Header)) / element_size;
    void* data = SDL_malloc(element_size * capacity + sizeof(_Array_Header));
    if (!data) return false;
    
    _Array_Header* header = (_Array_Header*)data;
    header->len = 0;
    header->capacity = capacity;
    header->element_size = element_size;
    header->allocator = NULL;
    
    *array = (char*)data + sizeof(_Array_Header);
    return true;
}

inline bool _Array_Append_Slow(void** array, void* element)
{
    _Array_Header* header = Array_Header(*array);

    // if (header->len >= header->capacity) /* no need for this check; it happens in the `Array_Append` macro */
    {
        size_t new_capacity = (size_t)(header->capacity * 1.5);
        if (new_capacity <= header->capacity) new_capacity = header->capacity + 1;
        
        void* new_block = SDL_realloc(header, header->element_size * new_capacity + sizeof(_Array_Header));
        if (!new_block) return false;

        header = (_Array_Header*)new_block;
        header->capacity = new_capacity;

        *array = (char*)new_block + sizeof(_Array_Header);
    }
    
    SDL_memcpy((char*)(*array) + header->len * header->element_size, element, header->element_size);
    header->len++;
    return true;
}

inline bool _Array_Insert(void** array, size_t index, void* element)
{
    _Array_Header* header = Array_Header(*array);
    if (index > header->len) return false;

    if (header->len >= header->capacity) 
    {
        size_t new_capacity = (size_t)(header->capacity * 1.5);
        if (new_capacity <= header->capacity) new_capacity = header->capacity + 1;
        
        void* new_block = SDL_realloc(header, header->element_size * new_capacity + sizeof(_Array_Header));
        if (!new_block) return false;

        header = (_Array_Header*)new_block;
        header->capacity = new_capacity;

        *array = (char*)new_block + sizeof(_Array_Header);
    }

    if (index < header->len)
    {
        SDL_memmove((char*)(*array) + (index + 1) * header->element_size,
                (char*)(*array) + index * header->element_size,
                (header->len - index) * header->element_size);
    }

    SDL_memcpy((char*)(*array) + index * header->element_size, element, header->element_size);
    header->len++;
    return true;
}

inline void* _Array_Pop(void* array)
{
    _Array_Header* header = Array_Header(array);
    if (header->len == 0) return NULL;
    header->len--;
    return (char*)array + header->len * header->element_size;
}

inline bool _Array_DeleteShift(void** array, size_t index)
{
    _Array_Header* header = Array_Header(*array);
    if (index >= header->len) return false;
    if (index < header->len - 1)
    {
        SDL_memmove((char*)(*array) + index * header->element_size,
                (char*)(*array) + (index + 1) * header->element_size,
                (header->len - index - 1) * header->element_size);
    }
    header->len--;
    return true;
}

inline bool _Array_DeleteSwap(void** array, size_t index)
{
    _Array_Header* header = Array_Header(*array);
    if (index >= header->len) return false;
    if (index < header->len - 1)
    {
        SDL_memcpy((char*)(*array) + index * header->element_size,
               (char*)(*array) + (header->len - 1) * header->element_size,
               header->element_size);
    }
    header->len--;
    return true;
}

inline bool _Array_SDL_free(void** array)
{
    if (!array || !*array) return false;
    _Array_Header* header = Array_Header(*array);
    SDL_free(header);
    *array = NULL;
    return true;
}

#endif // ARRAY_H