// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_OG_OG_STATUS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_OG_OG_STATUS_H_

#include <string>

#include "base/component_export.h"
#include "base/logging.h"
#include "brave/components/local_ai/og/platform_functions_og.h"

struct OgaResult;

namespace local_ai::og {

namespace internal {

COMPONENT_EXPORT(LOCAL_AI) std::string OgaResultErrorMessage(OgaResult* result);

}  // namespace internal

// Helper macro for calling OGA functions through platform functions
// Use this for direct function calls that don't return OgaResult
#define OGA_CALL(func) \
  ::local_ai::og::PlatformFunctions::GetInstance()->func##Ptr

// Macro to check OGA result and abort on error (for critical operations)
#define CHECK_OGA_RESULT(expr)                                 \
  if (OgaResult* result = (expr)) {                            \
    LOG(FATAL) << og::internal::OgaResultErrorMessage(result); \
  }

// Macro to call OGA function and check for errors (returns bool - true if
// failed)
#define OGA_CALL_FAILED(expr)                                    \
  ([&]() -> bool {                                               \
    OgaResult* result = (expr);                                  \
    if (result) {                                                \
      LOG(ERROR) << "[LocalAI] Failed to call " << #expr << ": " \
                 << og::internal::OgaResultErrorMessage(result); \
      return true;                                               \
    }                                                            \
    return false;                                                \
  })()

// Macro to call OGA function and return success (returns bool - true if
// succeeded)
#define OGA_CALL_SUCCESS(expr) !OGA_CALL_FAILED(expr)

// Macro to call OGA function and return error message on failure
#define CALL_OGA_WITH_MESSAGE(expr)                       \
  ([&]() -> std::string {                                 \
    OgaResult* result = (expr);                           \
    if (result) {                                         \
      return og::internal::OgaResultErrorMessage(result); \
    }                                                     \
    return std::string();                                 \
  })()

}  // namespace local_ai::og

#endif  // BRAVE_COMPONENTS_LOCAL_AI_OG_OG_STATUS_H_
