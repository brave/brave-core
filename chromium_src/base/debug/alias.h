/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_DEBUG_ALIAS_H_
#define BRAVE_CHROMIUM_SRC_BASE_DEBUG_ALIAS_H_

#include <stdint.h>
#include <string.h>

#include "base/memory/raw_ptr_exclusion.h"

#include "src/base/debug/alias.h"  // IWYU pragma: export

namespace base {
namespace debug {

template <typename T>
class StackObjectCopy {
 public:
  explicit StackObjectCopy(const T* original) {
    if (original) {
      memcpy(static_cast<void*>(buffer_), static_cast<const void*>(original),
             sizeof(T));
    } else {
      memset(buffer_, 0, sizeof(T));
    }
  }

  StackObjectCopy(const StackObjectCopy&) = delete;
  StackObjectCopy& operator=(const StackObjectCopy&) = delete;

 private:
  alignas(T) uint8_t buffer_[sizeof(T)];
  RAW_PTR_EXCLUSION const T* const typed_view_ =
      reinterpret_cast<const T*>(buffer_);
};

}  // namespace debug
}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_DEBUG_ALIAS_H_
