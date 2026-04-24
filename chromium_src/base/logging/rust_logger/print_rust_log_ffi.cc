/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/logging/rust_logger/print_rust_log_ffi.h"

#include "base/logging.h"

#define print_rust_log print_rust_log_chromium_impl
#include <base/logging/rust_logger/print_rust_log_ffi.cc>
#undef print_rust_log

namespace logging::internal {

void print_rust_log(const RustFmtArguments& msg,
                    const char* file,
                    int32_t line,
                    int32_t severity) {
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
      print_rust_log_chromium_impl(msg, file, line, severity);
      return;
  }

  if (!VLOG_IS_ON(-severity)) {
    return;
  }

  print_rust_log_chromium_impl(msg, file, line, severity);
}

}  // namespace logging::internal
