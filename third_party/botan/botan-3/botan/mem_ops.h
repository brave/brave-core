/*
* Memory Operations
* (C) 1999-2009,2012,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MEMORY_OPS_H_
#define BOTAN_MEMORY_OPS_H_

#include <botan/concepts.h>
#include <botan/types.h>
#include <array>
#include <cstring>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

/*
The header mem_ops.h previously included the contents of allocator.h

Library code should always include allocator.h to see these
declarations; however when we are not building the library continue to
include the header here to avoid breaking application code.
*/
#if !defined(BOTAN_IS_BEING_BUILT)
   #include <botan/allocator.h>
#endif

namespace Botan {

/**
* Scrub memory contents in a way that a compiler should not elide,
* using some system specific technique. Note that this function might
* not zero the memory (for example, in some hypothetical
* implementation it might combine the memory contents with the output
* of a system PRNG), but if you can detect any difference in behavior
* at runtime then the clearing is side-effecting and you can just
* use `clear_mem`.
*
* Use this function to scrub memory just before deallocating it, or on
* a stack buffer before returning from the function.
*
* @param ptr a pointer to memory to scrub
* @param n the number of bytes pointed to by ptr
*/
BOTAN_PUBLIC_API(2, 0) void secure_scrub_memory(void* ptr, size_t n);

/**
* Scrub memory contents in a way that a compiler should not elide,
* using some system specific technique. Note that this function might
* not zero the memory.
*
* @param data  the data region to be scrubbed
*/
void secure_scrub_memory(ranges::contiguous_output_range auto&& data) {
   secure_scrub_memory(std::ranges::data(data), ranges::size_bytes(data));
}

#if !defined(BOTAN_IS_BEGIN_BUILT)

/**
* Memory comparison, input insensitive
* @param x a pointer to an array
* @param y a pointer to another array
* @param len the number of Ts in x and y
* @return 0xFF iff x[i] == y[i] forall i in [0...n) or 0x00 otherwise
*/
BOTAN_DEPRECATED("This function is deprecated, use constant_time_compare()")
BOTAN_PUBLIC_API(2, 9) uint8_t ct_compare_u8(const uint8_t x[], const uint8_t y[], size_t len);

#endif

/**
 * Memory comparison, input insensitive
 * @param x a range of bytes
 * @param y another range of bytes
 * @return true iff x and y have equal lengths and x[i] == y[i] forall i in [0...n)
 */
BOTAN_PUBLIC_API(3, 3) bool constant_time_compare(std::span<const uint8_t> x, std::span<const uint8_t> y);

/**
* Memory comparison, input insensitive
* @param x a pointer to an array
* @param y a pointer to another array
* @param len the number of Ts in x and y
* @return true iff x[i] == y[i] forall i in [0...n)
*/
inline bool constant_time_compare(const uint8_t x[], const uint8_t y[], size_t len) {
   // simply assumes that *x and *y point to len allocated bytes at least
   return constant_time_compare({x, len}, {y, len});
}

/**
* Zero out some bytes. Warning: use secure_scrub_memory instead if the
* memory is about to be freed or otherwise the compiler thinks it can
* elide the writes.
*
* @param ptr a pointer to memory to zero
* @param bytes the number of bytes to zero in ptr
*/
inline constexpr void clear_bytes(void* ptr, size_t bytes) {
   if(bytes > 0) {
      std::memset(ptr, 0, bytes);
   }
}

/**
* Zero memory before use. This simply calls memset and should not be
* used in cases where the compiler cannot see the call as a
* side-effecting operation (for example, if calling clear_mem before
* deallocating memory, the compiler would be allowed to omit the call
* to memset entirely under the as-if rule.)
*
* @param ptr a pointer to an array of Ts to zero
* @param n the number of Ts pointed to by ptr
*/
template <typename T>
inline constexpr void clear_mem(T* ptr, size_t n) {
   clear_bytes(ptr, sizeof(T) * n);
}

/**
* Zero memory before use. This simply calls memset and should not be
* used in cases where the compiler cannot see the call as a
* side-effecting operation.
*
* @param mem a contiguous range of Ts to zero
*/
template <ranges::contiguous_output_range R>
inline constexpr void clear_mem(R&& mem)
   requires std::is_trivially_copyable_v<std::ranges::range_value_t<R>>
{
   clear_bytes(std::ranges::data(mem), ranges::size_bytes(mem));
}

/**
* Copy memory
* @param out the destination array
* @param in the source array
* @param n the number of elements of in/out
*/
template <typename T>
   requires std::is_trivial<typename std::decay<T>::type>::value
inline constexpr void copy_mem(T* out, const T* in, size_t n) {
   BOTAN_ASSERT_IMPLICATION(n > 0, in != nullptr && out != nullptr, "If n > 0 then args are not null");

   if(in != nullptr && out != nullptr && n > 0) {
      std::memmove(out, in, sizeof(T) * n);
   }
}

/**
* Copy memory
* @param out the destination array
* @param in the source array
*/
template <ranges::contiguous_output_range OutR, ranges::contiguous_range InR>
   requires std::is_same_v<std::ranges::range_value_t<OutR>, std::ranges::range_value_t<InR>> &&
            std::is_trivially_copyable_v<std::ranges::range_value_t<InR>>
inline constexpr void copy_mem(OutR&& out, InR&& in) {
   ranges::assert_equal_byte_lengths(out, in);
   if(std::is_constant_evaluated()) {
      std::copy(std::ranges::begin(in), std::ranges::end(in), std::ranges::begin(out));
   } else if(ranges::size_bytes(out) > 0) {
      std::memmove(std::ranges::data(out), std::ranges::data(in), ranges::size_bytes(out));
   }
}

/**
 * Copy a range of a trivially copyable type into another range of trivially
 * copyable type of matching byte length.
 */
template <ranges::contiguous_output_range ToR, ranges::contiguous_range FromR>
   requires std::is_trivially_copyable_v<std::ranges::range_value_t<FromR>> &&
            std::is_trivially_copyable_v<std::ranges::range_value_t<ToR>>
inline constexpr void typecast_copy(ToR&& out, FromR&& in) {
   ranges::assert_equal_byte_lengths(out, in);
   std::memcpy(std::ranges::data(out), std::ranges::data(in), ranges::size_bytes(out));
}

/**
 * Copy a range of trivially copyable type into an instance of trivially
 * copyable type with matching length.
 */
template <typename ToT, ranges::contiguous_range FromR>
   requires std::is_trivially_copyable_v<std::ranges::range_value_t<FromR>> && std::is_trivially_copyable_v<ToT> &&
            (!std::ranges::range<ToT>)
inline constexpr void typecast_copy(ToT& out, FromR&& in) noexcept {
   typecast_copy(std::span<ToT, 1>(&out, 1), in);
}

/**
 * Copy an instance of trivially copyable type into a range of trivially
 * copyable type with matching length.
 */
template <ranges::contiguous_output_range ToR, typename FromT>
   requires std::is_trivially_copyable_v<FromT> &&
            (!std::ranges::range<FromT>) && std::is_trivially_copyable_v<std::ranges::range_value_t<ToR>>
inline constexpr void typecast_copy(ToR&& out, const FromT& in) {
   typecast_copy(out, std::span<const FromT, 1>(&in, 1));
}

/**
 * Create a trivial type by bit-casting a range of trivially copyable type with
 * matching length into it.
 */
template <typename ToT, ranges::contiguous_range FromR>
   requires std::is_default_constructible_v<ToT> && std::is_trivially_copyable_v<ToT> &&
            std::is_trivially_copyable_v<std::ranges::range_value_t<FromR>>
inline constexpr ToT typecast_copy(FromR&& src) noexcept {
   ToT dst;
   typecast_copy(dst, src);
   return dst;
}

// TODO: deprecate and replace
template <typename T>
inline constexpr void typecast_copy(uint8_t out[], T in[], size_t N)
   requires std::is_trivially_copyable<T>::value
{
   // asserts that *in and *out point to the correct amount of memory
   typecast_copy(std::span<uint8_t>(out, sizeof(T) * N), std::span<const T>(in, N));
}

// TODO: deprecate and replace
template <typename T>
inline constexpr void typecast_copy(T out[], const uint8_t in[], size_t N)
   requires std::is_trivial<T>::value
{
   // asserts that *in and *out point to the correct amount of memory
   typecast_copy(std::span<T>(out, N), std::span<const uint8_t>(in, N * sizeof(T)));
}

// TODO: deprecate and replace
template <typename T>
inline constexpr void typecast_copy(uint8_t out[], const T& in) {
   // asserts that *out points to the correct amount of memory
   typecast_copy(std::span<uint8_t, sizeof(T)>(out, sizeof(T)), in);
}

// TODO: deprecate and replace
template <typename T>
   requires std::is_trivial<typename std::decay<T>::type>::value
inline constexpr void typecast_copy(T& out, const uint8_t in[]) {
   // asserts that *in points to the correct amount of memory
   typecast_copy(out, std::span<const uint8_t, sizeof(T)>(in, sizeof(T)));
}

// TODO: deprecate and replace
template <typename To>
   requires std::is_trivial<To>::value
inline constexpr To typecast_copy(const uint8_t src[]) noexcept {
   // asserts that *src points to the correct amount of memory
   return typecast_copy<To>(std::span<const uint8_t, sizeof(To)>(src, sizeof(To)));
}

#if !defined(BOTAN_IS_BEGIN_BUILT)
/**
* Set memory to a fixed value
* @param ptr a pointer to an array of bytes
* @param n the number of Ts pointed to by ptr
* @param val the value to set each byte to
*/
BOTAN_DEPRECATED("This function is deprecated") inline constexpr void set_mem(uint8_t* ptr, size_t n, uint8_t val) {
   if(n > 0) {
      std::memset(ptr, val, n);
   }
}
#endif

inline const uint8_t* cast_char_ptr_to_uint8(const char* s) {
   return reinterpret_cast<const uint8_t*>(s);
}

inline const char* cast_uint8_ptr_to_char(const uint8_t* b) {
   return reinterpret_cast<const char*>(b);
}

inline uint8_t* cast_char_ptr_to_uint8(char* s) {
   return reinterpret_cast<uint8_t*>(s);
}

inline char* cast_uint8_ptr_to_char(uint8_t* b) {
   return reinterpret_cast<char*>(b);
}

#if !defined(BOTAN_IS_BEING_BUILT)
/**
* Memory comparison, input insensitive
* @param p1 a pointer to an array
* @param p2 a pointer to another array
* @param n the number of Ts in p1 and p2
* @return true iff p1[i] == p2[i] forall i in [0...n)
*/
template <typename T>
BOTAN_DEPRECATED("This function is deprecated")
inline bool same_mem(const T* p1, const T* p2, size_t n) {
   volatile T difference = 0;

   for(size_t i = 0; i != n; ++i) {
      difference = difference | (p1[i] ^ p2[i]);
   }

   return difference == 0;
}
#endif

#if !defined(BOTAN_IS_BEING_BUILT)

template <typename T, typename Alloc>
BOTAN_DEPRECATED("The buffer_insert functions are deprecated")
size_t buffer_insert(std::vector<T, Alloc>& buf, size_t buf_offset, const T input[], size_t input_length) {
   BOTAN_ASSERT_NOMSG(buf_offset <= buf.size());
   const size_t to_copy = std::min(input_length, buf.size() - buf_offset);
   if(to_copy > 0) {
      copy_mem(&buf[buf_offset], input, to_copy);
   }
   return to_copy;
}

template <typename T, typename Alloc, typename Alloc2>
BOTAN_DEPRECATED("The buffer_insert functions are deprecated")
size_t buffer_insert(std::vector<T, Alloc>& buf, size_t buf_offset, const std::vector<T, Alloc2>& input) {
   BOTAN_ASSERT_NOMSG(buf_offset <= buf.size());
   const size_t to_copy = std::min(input.size(), buf.size() - buf_offset);
   if(to_copy > 0) {
      copy_mem(&buf[buf_offset], input.data(), to_copy);
   }
   return to_copy;
}

#endif

/**
* XOR arrays. Postcondition out[i] = in[i] ^ out[i] forall i = 0...length
* @param out the input/output range
* @param in the read-only input range
*/
inline constexpr void xor_buf(ranges::contiguous_output_range<uint8_t> auto&& out,
                              ranges::contiguous_range<uint8_t> auto&& in) {
   ranges::assert_equal_byte_lengths(out, in);

   std::span o{out};
   std::span i{in};

   for(; o.size_bytes() >= 32; o = o.subspan(32), i = i.subspan(32)) {
      auto x = typecast_copy<std::array<uint64_t, 4>>(o.template first<32>());
      const auto y = typecast_copy<std::array<uint64_t, 4>>(i.template first<32>());

      x[0] ^= y[0];
      x[1] ^= y[1];
      x[2] ^= y[2];
      x[3] ^= y[3];

      typecast_copy(o.template first<32>(), x);
   }

   for(size_t off = 0; off != o.size_bytes(); ++off) {
      o[off] ^= i[off];
   }
}

/**
* XOR arrays. Postcondition out[i] = in1[i] ^ in2[i] forall i = 0...length
* @param out the output range
* @param in1 the first input range
* @param in2 the second input range
*/
inline constexpr void xor_buf(ranges::contiguous_output_range<uint8_t> auto&& out,
                              ranges::contiguous_range<uint8_t> auto&& in1,
                              ranges::contiguous_range<uint8_t> auto&& in2) {
   ranges::assert_equal_byte_lengths(out, in1, in2);

   std::span o{out};
   std::span i1{in1};
   std::span i2{in2};

   for(; o.size_bytes() >= 32; o = o.subspan(32), i1 = i1.subspan(32), i2 = i2.subspan(32)) {
      auto x = typecast_copy<std::array<uint64_t, 4>>(i1.template first<32>());
      const auto y = typecast_copy<std::array<uint64_t, 4>>(i2.template first<32>());

      x[0] ^= y[0];
      x[1] ^= y[1];
      x[2] ^= y[2];
      x[3] ^= y[3];

      typecast_copy(o.template first<32>(), x);
   }

   for(size_t off = 0; off != o.size_bytes(); ++off) {
      o[off] = i1[off] ^ i2[off];
   }
}

/**
* XOR arrays. Postcondition out[i] = in[i] ^ out[i] forall i = 0...length
* @param out the input/output buffer
* @param in the read-only input buffer
* @param length the length of the buffers
*/
inline void xor_buf(uint8_t out[], const uint8_t in[], size_t length) {
   // simply assumes that *out and *in point to "length" allocated bytes at least
   xor_buf(std::span{out, length}, std::span{in, length});
}

/**
* XOR arrays. Postcondition out[i] = in[i] ^ in2[i] forall i = 0...length
* @param out the output buffer
* @param in the first input buffer
* @param in2 the second input buffer
* @param length the length of the three buffers
*/
inline void xor_buf(uint8_t out[], const uint8_t in[], const uint8_t in2[], size_t length) {
   // simply assumes that *out, *in, and *in2 point to "length" allocated bytes at least
   xor_buf(std::span{out, length}, std::span{in, length}, std::span{in2, length});
}

// TODO: deprecate and replace, use .subspan()
inline void xor_buf(std::span<uint8_t> out, std::span<const uint8_t> in, size_t n) {
   BOTAN_ARG_CHECK(out.size() >= n, "output span is too small");
   BOTAN_ARG_CHECK(in.size() >= n, "input span is too small");
   xor_buf(out.first(n), in.first(n));
}

// TODO: deprecate and replace, use .subspan()
template <typename Alloc>
void xor_buf(std::vector<uint8_t, Alloc>& out, const uint8_t* in, size_t n) {
   BOTAN_ARG_CHECK(out.size() >= n, "output vector is too small");
   // simply assumes that *in points to "n" allocated bytes at least
   xor_buf(std::span{out}.first(n), std::span{in, n});
}

// TODO: deprecate and replace
template <typename Alloc, typename Alloc2>
void xor_buf(std::vector<uint8_t, Alloc>& out, const uint8_t* in, const std::vector<uint8_t, Alloc2>& in2, size_t n) {
   BOTAN_ARG_CHECK(out.size() >= n, "output vector is too small");
   BOTAN_ARG_CHECK(in2.size() >= n, "input vector is too small");
   // simply assumes that *in points to "n" allocated bytes at least
   xor_buf(std::span{out}.first(n), std::span{in, n}, std::span{in2}.first(n));
}

template <typename Alloc, typename Alloc2>
std::vector<uint8_t, Alloc>& operator^=(std::vector<uint8_t, Alloc>& out, const std::vector<uint8_t, Alloc2>& in) {
   if(out.size() < in.size()) {
      out.resize(in.size());
   }

   xor_buf(std::span{out}.first(in.size()), in);
   return out;
}

}  // namespace Botan

#endif
