/*
* Runtime assertion checking
* (C) 2010,2018 Jack Lloyd
*     2017 Simon Warta (Kullo GmbH)
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ASSERTION_CHECKING_H_
#define BOTAN_ASSERTION_CHECKING_H_

#include <botan/compiler.h>

namespace Botan {

/**
* Called when an assertion fails
* Throws an Exception object
*/
[[noreturn]] void BOTAN_PUBLIC_API(2, 0)
   assertion_failure(const char* expr_str, const char* assertion_made, const char* func, const char* file, int line);

/**
* Called when an invalid argument is used
* Throws Invalid_Argument
*/
[[noreturn]] void BOTAN_UNSTABLE_API throw_invalid_argument(const char* message, const char* func, const char* file);

#define BOTAN_ARG_CHECK(expr, msg)                               \
   do {                                                          \
      if(!(expr))                                                \
         Botan::throw_invalid_argument(msg, __func__, __FILE__); \
   } while(0)

/**
* Called when an invalid state is encountered
* Throws Invalid_State
*/
[[noreturn]] void BOTAN_UNSTABLE_API throw_invalid_state(const char* message, const char* func, const char* file);

#define BOTAN_STATE_CHECK(expr)                                 \
   do {                                                         \
      if(!(expr))                                               \
         Botan::throw_invalid_state(#expr, __func__, __FILE__); \
   } while(0)

/**
* Make an assertion
*/
#define BOTAN_ASSERT(expr, assertion_made)                                              \
   do {                                                                                 \
      if(!(expr))                                                                       \
         Botan::assertion_failure(#expr, assertion_made, __func__, __FILE__, __LINE__); \
   } while(0)

/**
* Make an assertion
*/
#define BOTAN_ASSERT_NOMSG(expr)                                            \
   do {                                                                     \
      if(!(expr))                                                           \
         Botan::assertion_failure(#expr, "", __func__, __FILE__, __LINE__); \
   } while(0)

/**
* Assert that value1 == value2
*/
#define BOTAN_ASSERT_EQUAL(expr1, expr2, assertion_made)                                               \
   do {                                                                                                \
      if((expr1) != (expr2))                                                                           \
         Botan::assertion_failure(#expr1 " == " #expr2, assertion_made, __func__, __FILE__, __LINE__); \
   } while(0)

/**
* Assert that expr1 (if true) implies expr2 is also true
*/
#define BOTAN_ASSERT_IMPLICATION(expr1, expr2, msg)                                              \
   do {                                                                                          \
      if((expr1) && !(expr2))                                                                    \
         Botan::assertion_failure(#expr1 " implies " #expr2, msg, __func__, __FILE__, __LINE__); \
   } while(0)

/**
* Assert that a pointer is not null
*/
#define BOTAN_ASSERT_NONNULL(ptr)                                                         \
   do {                                                                                   \
      if((ptr) == nullptr)                                                                \
         Botan::assertion_failure(#ptr " is not null", "", __func__, __FILE__, __LINE__); \
   } while(0)

#if defined(BOTAN_ENABLE_DEBUG_ASSERTS)

   #define BOTAN_DEBUG_ASSERT(expr) BOTAN_ASSERT_NOMSG(expr)

#else

   #define BOTAN_DEBUG_ASSERT(expr) \
      do {                          \
      } while(0)

#endif

/**
* Mark variable as unused.
*
* Takes any number of arguments and marks all as unused, for instance
* BOTAN_UNUSED(a); or BOTAN_UNUSED(x, y, z);
*/
template <typename T>
constexpr void ignore_param(T&&) {}

template <typename... T>
constexpr void ignore_params(T&&... args) {
   (ignore_param(args), ...);
}

#define BOTAN_UNUSED Botan::ignore_params

/*
* Define Botan::assert_unreachable and BOTAN_ASSERT_UNREACHABLE
*
* This is intended to be used in the same situations as `std::unreachable()`;
* a codepath that (should not) be reachable but where the compiler cannot
* tell that it is unreachable.
*
* Unlike `std::unreachable()`, or equivalent compiler builtins like GCC's
* `__builtin_unreachable`, this function is not UB. By default it will
* throw an exception. If `BOTAN_TERMINATE_ON_ASSERTS` is defined, it will
* instead print a message to stderr and abort.
*
* Due to this difference, and the fact that it is not inlined, calling
* this is significantly more costly than using `std::unreachable`.
*/
[[noreturn]] void BOTAN_UNSTABLE_API assert_unreachable(const char* file, int line);

#define BOTAN_ASSERT_UNREACHABLE() Botan::assert_unreachable(__FILE__, __LINE__)

}  // namespace Botan

#endif
