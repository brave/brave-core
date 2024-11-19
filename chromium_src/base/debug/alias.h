/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_DEBUG_ALIAS_H_
#define BRAVE_CHROMIUM_SRC_BASE_DEBUG_ALIAS_H_

#include <stdint.h>

#include "base/containers/span.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/memory/stack_allocated.h"
#include "base/ranges/algorithm.h"

#include "src/base/debug/alias.h"  // IWYU pragma: export

namespace base {
namespace debug {

template <typename T>
class StackObjectCopy {
  STACK_ALLOCATED();

 public:
  explicit StackObjectCopy(const T* original) {
    if (original) {
      base::span(buffer_).copy_from(base::byte_span_from_ref(*original));
    } else {
      base::ranges::fill(buffer_, 0);
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
