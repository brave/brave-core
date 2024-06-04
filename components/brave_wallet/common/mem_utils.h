/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MEM_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MEM_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include "base/containers/span.h"

namespace brave_wallet {

// When we call memset in end of function to clean local variables
// for security reason, compiler optimizer can remove such call.
// So we use our own function for this purpose.
void SecureZeroData(base::span<uint8_t> bytes);

// Allocator which will zero out memory when destruct
template <typename T>
struct SecureZeroAllocator {
  SecureZeroAllocator() = default;
  ~SecureZeroAllocator() = default;
  SecureZeroAllocator(const SecureZeroAllocator&) = default;
  SecureZeroAllocator& operator=(const SecureZeroAllocator&) = default;
  SecureZeroAllocator(SecureZeroAllocator&&) = default;
  SecureZeroAllocator& operator=(SecureZeroAllocator&&) = default;

  using value_type = T;
  T* allocate(size_t n) {
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, size_t n) {
    SecureZeroData(
        base::as_writable_bytes(UNSAFE_BUFFERS(base::make_span(p, n))));
    ::operator delete(p);
  }
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MEM_UTILS_H_
