/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"

#include "base/logging.h"
#include "base/logging/logging_settings.h"

namespace brave_vpn {
namespace v2 {
namespace app_utils {
namespace {
inline constexpr char kVpnAppLogFile[] = "log-file";
}

void InitLogging(const base::CommandLine& command_line) {
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  base::FilePath log_file_path;
  if (command_line.HasSwitch(kVpnAppLogFile)) {
    settings.logging_dest |= logging::LOG_TO_FILE;
    log_file_path = command_line.GetSwitchValuePath(kVpnAppLogFile);
    settings.log_file_path = log_file_path.value().c_str();
  }
  logging::InitLogging(settings);
}

}  // namespace app_utils
}  // namespace v2
}  // namespace brave_vpn
