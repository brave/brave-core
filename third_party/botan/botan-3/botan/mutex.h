/*
* (C) 2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_UTIL_MUTEX_H_
#define BOTAN_UTIL_MUTEX_H_

#include <botan/types.h>

#if defined(BOTAN_TARGET_OS_HAS_THREADS)
   #include <mutex>
#endif

namespace Botan {

#if defined(BOTAN_TARGET_OS_HAS_THREADS)

using mutex_type = std::mutex;
using recursive_mutex_type = std::recursive_mutex;

template <typename T>
using lock_guard_type = std::lock_guard<T>;

#else

// No threads

class noop_mutex final {
   public:
      void lock() {}

      void unlock() {}
};

using mutex_type = noop_mutex;
using recursive_mutex_type = noop_mutex;

template <typename Mutex>
class lock_guard final {
   public:
      explicit lock_guard(Mutex& m) : m_mutex(m) { m_mutex.lock(); }

      ~lock_guard() { m_mutex.unlock(); }

      lock_guard(const lock_guard& other) = delete;
      lock_guard& operator=(const lock_guard& other) = delete;

   private:
      Mutex& m_mutex;
};

template <typename T>
using lock_guard_type = lock_guard<T>;

#endif

}  // namespace Botan

#endif
