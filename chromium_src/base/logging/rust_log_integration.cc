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
                                enum RustLogSeverity severity) {
  logging::LogSeverity log_severity = 0;
  switch (severity) {
    // Trace and debug logs are considered verbose and VLOGed.
    case RustLogSeverity::TRACE:
      log_severity = -4;
      break;
    case RustLogSeverity::DEBUG:
      log_severity = -3;
      break;
    default:
      // All other cases are handled by the upstream version.
      print_rust_log_chromium_impl(msg, file, line, severity);
      return;
  }

  if (!VLOG_IS_ON(-log_severity)) {
    return;
  }

  logging::LogMessage log_message(file, line, log_severity);
  log_message.stream() << msg;
}

}  // namespace internal
}  // namespace logging
