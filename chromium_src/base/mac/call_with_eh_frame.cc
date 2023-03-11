/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/mac/call_with_eh_frame.h"

#include <stdint.h>
#include <unwind.h>

#include "base/base_export.h"
#include "build/build_config.h"
#include "src/base/mac/call_with_eh_frame.cc"

// Rename the chromium personality routine to match the one included with rust
// so we can override it at link time.
// See https://github.com/rust-lang/rust/issues/102754#issuecomment-1399669725
#if defined(__x86_64__) || defined(__aarch64__)

extern "C" BASE_EXPORT _Unwind_Reason_Code
rust_eh_personality_impl(int version,
                         _Unwind_Action actions,
                         uint64_t exception_class,
                         struct _Unwind_Exception* exception_object,
                         struct _Unwind_Context* context) {
  return base::mac::CxxPersonalityRoutine(version, actions, exception_class,
                                          exception_object, context);
}

extern "C" __attribute__((weak)) _Unwind_Reason_Code rust_eh_personality(
    int version,
    _Unwind_Action actions,
    uint64_t exception_class,
    struct _Unwind_Exception* exception_object,
    struct _Unwind_Context* context) {
  return base::mac::CxxPersonalityRoutine(version, actions, exception_class,
                                          exception_object, context);
}

#endif  // defined(__x86_64__) || defined(__aarch64__)
