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

BASE_EXPORT void print_rust_log(const char* msg,
                                const char* file,
                                int line,
                                LogSeverity severity,
                                bool verbose) {
  switch (severity) {
    // Trace and debug logs are set as `LOGGING_INFO`. Trace is also set as
    // versbose, so we make a higher level verbosity.
    case LOGGING_INFO:
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

  logging::LogMessage log_message(file, line, severity);
  log_message.stream() << msg;
}

}  // namespace internal
}  // namespace logging
