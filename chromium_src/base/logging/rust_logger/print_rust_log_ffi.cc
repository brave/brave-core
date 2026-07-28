/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/logging/rust_logger/print_rust_log_ffi.h"

#include "base/logging.h"

namespace logging::internal {

namespace {

bool ShouldPrintRustLog(int32_t& severity) {
  switch (severity) {
    // Trace and debug logs are set as `LOGGING_INFO`. We also map
    // `LOGGING_WARN` to verbose, to avoid "excessive output" errors in unit
    // tests.
    case LOGGING_INFO:
    case LOGGING_WARNING:
      severity = LOGGING_VERBOSE;
      break;
    default:
      // All other cases are handled by the upstream version.
      return true;
  }

  return VLOG_IS_ON(-severity);
}

}  // namespace

}  // namespace logging::internal

#include <base/logging/rust_logger/print_rust_log_ffi.cc>
