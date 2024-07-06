/*
* Secure Memory Buffers
* (C) 1999-2007,2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SECURE_MEMORY_BUFFERS_H_
#define BOTAN_SECURE_MEMORY_BUFFERS_H_

#include <botan/allocator.h>
#include <botan/types.h>  // IWYU pragma: export
#include <algorithm>
#include <deque>
#include <type_traits>
#include <vector>  // IWYU pragma: export

namespace Botan {

template <typename T>
#if !defined(_ITERATOR_DEBUG_LEVEL) || _ITERATOR_DEBUG_LEVEL == 0
/*
  * Assert exists to prevent someone from doing something that will
  * probably crash anyway (like secure_vector<non_POD_t> where ~non_POD_t
  * deletes a member pointer which was zeroed before it ran).
  * MSVC in debug mode uses non-integral proxy types in container types
  * like std::vector, thus we disable the check there.
 */
   requires std::is_integral<T>::value
#endif
class secure_allocator {

   public:
      typedef T value_type;
      typedef std::size_t size_type;

      secure_allocator() noexcept = default;
      secure_allocator(const secure_allocator&) noexcept = default;
      secure_allocator& operator=(const secure_allocator&) noexcept = default;
      ~secure_allocator() noexcept = default;

      template <typename U>
      secure_allocator(const secure_allocator<U>&) noexcept {}

      T* allocate(std::size_t n) { return static_cast<T*>(allocate_memory(n, sizeof(T))); }

      void deallocate(T* p, std::size_t n) { deallocate_memory(p, n, sizeof(T)); }
};

template <typename T, typename U>
inline bool operator==(const secure_allocator<T>&, const secure_allocator<U>&) {
   return true;
}

template <typename T, typename U>
inline bool operator!=(const secure_allocator<T>&, const secure_allocator<U>&) {
   return false;
}

template <typename T>
using secure_vector = std::vector<T, secure_allocator<T>>;
template <typename T>
using secure_deque = std::deque<T, secure_allocator<T>>;

// For better compatibility with 1.10 API
template <typename T>
using SecureVector = secure_vector<T>;

template <typename T>
secure_vector<T> lock(const std::vector<T>& in) {
   return secure_vector<T>(in.begin(), in.end());
}

template <typename T>
std::vector<T> unlock(const secure_vector<T>& in) {
   return std::vector<T>(in.begin(), in.end());
}

template <typename T, typename Alloc, typename Alloc2>
std::vector<T, Alloc>& operator+=(std::vector<T, Alloc>& out, const std::vector<T, Alloc2>& in) {
   out.insert(out.end(), in.begin(), in.end());
   return out;
}

template <typename T, typename Alloc>
std::vector<T, Alloc>& operator+=(std::vector<T, Alloc>& out, T in) {
   out.push_back(in);
   return out;
}

template <typename T, typename Alloc, typename L>
std::vector<T, Alloc>& operator+=(std::vector<T, Alloc>& out, const std::pair<const T*, L>& in) {
   out.insert(out.end(), in.first, in.first + in.second);
   return out;
}

template <typename T, typename Alloc, typename L>
std::vector<T, Alloc>& operator+=(std::vector<T, Alloc>& out, const std::pair<T*, L>& in) {
   out.insert(out.end(), in.first, in.first + in.second);
   return out;
}

/**
* Zeroise the values; length remains unchanged
* @param vec the vector to zeroise
*/
template <typename T, typename Alloc>
void zeroise(std::vector<T, Alloc>& vec) {
   std::fill(vec.begin(), vec.end(), static_cast<T>(0));
}

/**
* Zeroise the values then free the memory
* @param vec the vector to zeroise and free
*/
template <typename T, typename Alloc>
void zap(std::vector<T, Alloc>& vec) {
   zeroise(vec);
   vec.clear();
   vec.shrink_to_fit();
}

}  // namespace Botan

#endif
