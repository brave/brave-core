/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/logging/rust_log_integration.h"

#define print_rust_log print_rust_log_chromium_impl
#include "src/base/logging/rust_log_integration.cc"
#undef print_rust_log

namespace logging {
namespace internal {

void print_rust_log(const RustFmtArguments& msg,
                    const char* file,
                    int32_t line,
                    int32_t severity,
                    bool verbose) {
  switch (severity) {
    // Trace and debug logs are set as `LOGGING_INFO`. Trace is also set as
    // verbose, so we make a higher level verbosity. We also map `LOGGING_WARN`
    // to verbose, to avoid "excessive output" errors in unit tests.
    case LOGGING_INFO:
    case LOGGING_WARNING:
      severity = LOGGING_VERBOSE - verbose;
      break;
    default:
      // All other cases are handled by the upstream version.
      print_rust_log_chromium_impl(msg, file, line, severity, verbose);
      return;
  }

  if (!VLOG_IS_ON(-severity)) {
    return;
  }

  print_rust_log_chromium_impl(msg, file, line, severity, verbose);
}

}  // namespace internal
}  // namespace logging
