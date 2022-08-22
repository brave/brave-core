/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MEM_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MEM_UTILS_H_

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace brave_wallet {

// When we call memset in end of function to clean local variables
// for security reason, compiler optimizer can remove such call.
// So we use our own function for this purpose.
void SecureZeroData(void* data, size_t size);

// Allocator which will zero out memory when destruct
template <typename T>
struct SecureZeroAllocator {
  SecureZeroAllocator() = default;
  using value_type = T;
  T* allocate(size_t n) {
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, size_t n) {
    SecureZeroData(p, n);
    ::operator delete(p);
  }
};

// Deleter for std::vector to zero out memory when destruct
template <typename T>
struct SecureZeroVectorDeleter {
  SecureZeroVectorDeleter() = default;
  void operator()(std::vector<T>* p) const {
    SecureZeroData(p->data(), p->size());
    ::operator delete(p);
  }
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MEM_UTILS_H_
