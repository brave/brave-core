// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_ORT_ORT_STATUS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_ORT_ORT_STATUS_H_

#include <string>

#include "base/logging.h"
#include "brave/components/local_ai/ort/scoped_ort_types.h"

struct OrtStatus;

namespace local_ai::ort {

namespace internal {

COMPONENT_EXPORT(LOCAL_AI) std::string OrtStatusErrorMessage(OrtStatus* status);

}  // namespace internal

// Macro to check ORT status and abort on error (for critical operations)
#define CHECK_ORT_STATUS(expr)                                  \
  if (OrtStatus* status = (expr)) {                             \
    LOG(FATAL) << ort::internal::OrtStatusErrorMessage(status); \
  }

// Macro to call ORT function and return a scoped status (for error handling)
#define CALL_ORT_FUNC(expr)                                             \
  ([&]() -> ort::ScopedOrtStatus {                                      \
    ort::ScopedOrtStatus status(expr);                                  \
    if (status.is_valid()) {                                            \
      LOG(ERROR) << "[LocalAI] Failed to call " << #expr << ": "        \
                 << ort::internal::OrtStatusErrorMessage(status.get()); \
    }                                                                   \
    return status;                                                      \
  })()

// Macro to check if ORT call failed (returns bool)
#define ORT_CALL_FAILED(expr)                         \
  ([&]() -> bool {                                    \
    ort::ScopedOrtStatus status(CALL_ORT_FUNC(expr)); \
    if (status.is_valid()) {                          \
      return true;                                    \
    }                                                 \
    return false;                                     \
  })()

// Macro to call ORT function and return error message on failure
#define CALL_ORT_WITH_MESSAGE(expr)                              \
  ([&]() -> std::string {                                        \
    ort::ScopedOrtStatus status(expr);                           \
    if (status.is_valid()) {                                     \
      return ort::internal::OrtStatusErrorMessage(status.get()); \
    }                                                            \
    return std::string();                                        \
  })()

}  // namespace local_ai::ort

#endif  // BRAVE_COMPONENTS_LOCAL_AI_ORT_ORT_STATUS_H_
