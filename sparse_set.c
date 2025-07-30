#include "sparse_set.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool ensure_page_exists(PaginatedSparseSet* set, unsigned int page_index);

void sparse_set_init(PaginatedSparseSet* set) 
{
    if (!set) return;
    set->dense = NULL;
    set->data = NULL;
    set->sparse_pages = NULL;
    set->count = 0;
    set->dense_capacity = 0;
    set->page_capacity = 0;
}

void sparse_set_destroy(PaginatedSparseSet* set) 
{
    if (!set) return;

    free(set->dense);
    free(set->data);

    if (set->sparse_pages) 
    {
        for (size_t i = 0; i < set->page_capacity; i++) 
        {
            free(set->sparse_pages[i]);
        }
        free(set->sparse_pages);
    }
    
    sparse_set_init(set);
}

bool sparse_set_has(const PaginatedSparseSet* set, unsigned int id) 
{
    if (!set) return false;

    const unsigned int page_index = id >> SPARSE_SET_PAGE_SHIFT;
    const unsigned int page_offset = id & SPARSE_SET_PAGE_MASK;

    if (page_index >= set->page_capacity || set->sparse_pages[page_index] == NULL) 
    {
        return false;
    }

    const unsigned int dense_index = set->sparse_pages[page_index][page_offset];
    return dense_index < set->count && set->dense[dense_index] == id;
}


bool sparse_set_add(PaginatedSparseSet* set, unsigned int id, T value) 
{
    if (!set) return false;

    // item already exists; update its value.
    if (sparse_set_has(set, id)) 
    {
        T* data_ptr = sparse_set_get_data(set, id);
        if (data_ptr) 
        {
            *data_ptr = value;
        }
        return true;
    }

    if (set->count >= set->dense_capacity) 
    {
        size_t new_capacity = set->dense_capacity == 0 ? 8 : set->dense_capacity * 2;
        unsigned int* new_dense = realloc(set->dense, new_capacity * sizeof(unsigned int));
        T* new_data = realloc(set->data, new_capacity * sizeof(T));

        if (!new_dense || !new_data) 
        {
            free(new_dense); 
            free(new_data);
            return false;
        }
        set->dense = new_dense;
        set->data = new_data;
        set->dense_capacity = new_capacity;
    }

    const unsigned int page_index = id >> SPARSE_SET_PAGE_SHIFT;
    if (!ensure_page_exists(set, page_index)) 
    {
        return false; // Memory allocation failed
    }

    // Perform the add operation
    const unsigned int page_offset = id & SPARSE_SET_PAGE_MASK;
    const unsigned int dense_index = set->count;

    set->dense[dense_index] = id;
    set->data[dense_index] = value;
    set->sparse_pages[page_index][page_offset] = dense_index;

    set->count++;
    return true;
}

void sparse_set_remove(PaginatedSparseSet* set, unsigned int id) 
{
    if (!set || !sparse_set_has(set, id)) 
    {
        return;
    }

    const unsigned int page_index_to_remove = id >> SPARSE_SET_PAGE_SHIFT;
    const unsigned int page_offset_to_remove = id & SPARSE_SET_PAGE_MASK;

    // Get the dense index of the item we are removing
    unsigned int dense_index_to_remove = set->sparse_pages[page_index_to_remove][page_offset_to_remove];

    // Get the last item in the dense array
    unsigned int last_dense_index = set->count - 1;
    unsigned int last_id = set->dense[last_dense_index];
    T last_data = set->data[last_dense_index];

    // Move the last item into the slot of the removed item
    set->dense[dense_index_to_remove] = last_id;
    set->data[dense_index_to_remove] = last_data;

    // Update the sparse entry for the moved item to point to its new location
    const unsigned int moved_page_index = last_id >> SPARSE_SET_PAGE_SHIFT;
    const unsigned int moved_page_offset = last_id & SPARSE_SET_PAGE_MASK;
    set->sparse_pages[moved_page_index][moved_page_offset] = dense_index_to_remove;

    // Decrease the count
    set->count--;
}


T* sparse_set_get_data(PaginatedSparseSet* set, unsigned int id) 
{
    if (!set) return NULL;

    const unsigned int page_index = id >> SPARSE_SET_PAGE_SHIFT;
    const unsigned int page_offset = id & SPARSE_SET_PAGE_MASK;

    if (page_index >= set->page_capacity || set->sparse_pages[page_index] == NULL) 
    {
        return NULL;
    }

    const unsigned int dense_index = set->sparse_pages[page_index][page_offset];
    if (dense_index < set->count && set->dense[dense_index] == id) 
    {
        return &set->data[dense_index];
    }

    return NULL;
}

void sparse_set_clear(PaginatedSparseSet* set) 
{
    if (!set) return;
    set->count = 0;
}


// --- Internal Helper Functions ---

/**
 * @brief Ensures the sparse page table and the specific page are allocated.
 * @return true on success, false on memory allocation failure.
 */
static bool ensure_page_exists(PaginatedSparseSet* set, unsigned int page_index) 
{
    if (page_index >= set->page_capacity) 
    {
        size_t new_capacity = set->page_capacity;
        if (new_capacity == 0) new_capacity = 1;
        while (page_index >= new_capacity) {
            new_capacity *= 2;
        }

        unsigned int** new_pages = realloc(set->sparse_pages, new_capacity * sizeof(unsigned int*));
        if (!new_pages) return false;

        // Important: Initialize new page pointers to NULL
        memset(new_pages + set->page_capacity, 0, (new_capacity - set->page_capacity) * sizeof(unsigned int*));
        
        set->sparse_pages = new_pages;
        set->page_capacity = new_capacity;
    }

    if (set->sparse_pages[page_index] == NULL) 
    {
        unsigned int* new_page = malloc(SPARSE_SET_PAGE_SIZE * sizeof(unsigned int));
        if (!new_page) return false;
        
        // No need to memset; stale values are fine because sparse_set_has checks against set->count
        set->sparse_pages[page_index] = new_page;
    }

    return true;
}
