#include <SDL3/SDL.h>
#include "array.h"

// #ifdef ARRAY_IMPLEMENTATION

bool _Array_Init(void** array, size_t element_size, size_t capacity)
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

bool _Array_Append_Slow(void** array, void* element)
{
    _Array_Header* header = Array_Header(*array);

    // if (header->len >= header->capacity) 
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

bool _Array_Insert(void** array, size_t index, void* element)
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

void* _Array_Pop(void* array)
{
    _Array_Header* header = Array_Header(array);
    if (header->len == 0) return NULL;
    header->len--;
    return (char*)array + header->len * header->element_size;
}

bool _Array_DeleteShift(void** array, size_t index)
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

bool _Array_DeleteSwap(void** array, size_t index)
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

bool _Array_SDL_free(void** array)
{
    if (!array || !*array) return false;
    _Array_Header* header = Array_Header(*array);
    SDL_free(header);
    *array = NULL;
    return true;
}

// #endif // ARRAY_IMPLEMENTATION