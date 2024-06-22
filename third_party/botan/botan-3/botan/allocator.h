/*
* (C) 2023 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ALLOCATOR_HELPERS_H_
#define BOTAN_ALLOCATOR_HELPERS_H_

#include <botan/types.h>
#include <cstring>

namespace Botan {

/**
* Allocate a memory buffer by some method. This should only be used for
* primitive types (uint8_t, uint32_t, etc).
*
* @param elems the number of elements
* @param elem_size the size of each element
* @return pointer to allocated and zeroed memory, or throw std::bad_alloc on failure
*/
BOTAN_PUBLIC_API(2, 3) BOTAN_MALLOC_FN void* allocate_memory(size_t elems, size_t elem_size);

/**
* Free a pointer returned by allocate_memory
* @param p the pointer returned by allocate_memory
* @param elems the number of elements, as passed to allocate_memory
* @param elem_size the size of each element, as passed to allocate_memory
*/
BOTAN_PUBLIC_API(2, 3) void deallocate_memory(void* p, size_t elems, size_t elem_size);

/**
* Ensure the allocator is initialized
*/
void BOTAN_UNSTABLE_API initialize_allocator();

class Allocator_Initializer final {
   public:
      Allocator_Initializer() { initialize_allocator(); }
};

}  // namespace Botan

#endif
