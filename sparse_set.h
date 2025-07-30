#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stddef.h>
#include <stdbool.h>

// --- Configuration ---
// Define your data type 'T' before including this header.
// If not defined, it defaults to 'int'.
#ifndef T
#define T int
#endif

// The size of each page in the sparse array. Must be a power of 2.
// 4096 entries per page (16KB for unsigned int) is a common choice.
#define SPARSE_SET_PAGE_SIZE 4096
// --- End Configuration ---

// --- Internal constants derived from configuration ---
// Used for bitwise operations to calculate page index and offset quickly.
// e.g., if PAGE_SIZE is 4096 (2^12), PAGE_SHIFT is 12.
#if (SPARSE_SET_PAGE_SIZE & (SPARSE_SET_PAGE_SIZE - 1)) != 0
#error "SPARSE_SET_PAGE_SIZE must be a power of 2"
#endif

// A trick to calculate log2 for a power-of-2 number at compile time
#define SPARSE_SET_CTZ(N) ((N & 0x1)   ? 0 : (N & 0x2)   ? 1 : (N & 0x4)   ? 2 : \
                    (N & 0x8)   ? 3 : (N & 0x10)  ? 4 : (N & 0x20)  ? 5 : \
                    (N & 0x40)  ? 6 : (N & 0x80)  ? 7 : (N & 0x100) ? 8 : \
                    (N & 0x200) ? 9 : (N & 0x400) ? 10 : (N & 0x800) ? 11 : \
                    (N & 0x1000)? 12 : (N & 0x2000)? 13 : (N & 0x4000)? 14: 15)
#define SPARSE_SET_PAGE_SHIFT SPARSE_SET_CTZ(SPARSE_SET_PAGE_SIZE)
#define SPARSE_SET_PAGE_MASK (SPARSE_SET_PAGE_SIZE - 1)


/**
 * @brief The main structure for the paginated sparse set.
 */
typedef struct {
    // DENSE ARRAYS
    unsigned int* dense; // Stores item IDs contiguously.
    T* data;             // Stores data associated with each ID.

    // SPARSE ARRAY (PAGINATED)
    // A table of pointers to pages.
    unsigned int** sparse_pages; 

    // METADATA
    size_t count;           // Number of items currently in the set.
    size_t dense_capacity;  // Allocated capacity of the dense/data arrays.
    size_t page_capacity;   // Allocated capacity of the sparse_pages table.
} PaginatedSparseSet;


/**
 * @brief Initializes a paginated sparse set.
 *
 * @param set A pointer to the set to initialize.
 */
void sparse_set_init(PaginatedSparseSet* set);

/**
 * @brief Frees all memory associated with the sparse set.
 *
 * @param set A pointer to the set to destroy.
 */
void sparse_set_destroy(PaginatedSparseSet* set);

/**
 * @brief Adds an item (ID and data) to the set.
 * If the item already exists, its data is updated.
 *
 * @param set A pointer to the set.
 * @param id The item ID to add.
 * @param value The data to associate with the ID.
 * @return true on success, false on memory allocation failure.
 */
bool sparse_set_add(PaginatedSparseSet* set, unsigned int id, T value);

/**
 * @brief Removes an item from the set.
 *
 * @param set A pointer to the set.
 * @param id The item ID to remove.
 */
void sparse_set_remove(PaginatedSparseSet* set, unsigned int id);

/**
 * @brief Checks if an item exists in the set.
 *
 * @param set A pointer to the set.
 * @param id The item ID to check.
 * @return true if the item exists, false otherwise.
 */
bool sparse_set_has(const PaginatedSparseSet* set, unsigned int id);

/**
 * @brief Gets a pointer to the data associated with an ID.
 *
 * This allows for direct modification of the data.
 *
 * @param set A pointer to the set.
 * @param id The item ID whose data to retrieve.
 * @return A pointer to the data on success, or NULL if the item does not exist.
 */
T* sparse_set_get_data(PaginatedSparseSet* set, unsigned int id);

/**
 * @brief Clears all items from the set, but retains allocated memory.
 *
 * @param set A pointer to the set.
 */
void sparse_set_clear(PaginatedSparseSet* set);

/**
 * @brief Gets the number of items in the set.
 *
 * @param set A pointer to the set.
 * @return The number of items.
 */
static inline size_t sparse_set_count(const PaginatedSparseSet* set) {
    return set->count;
}

#endif // SPARSE_SET_H
