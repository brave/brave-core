/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/v2/app/shared/app_utils.h"

#include "base/base_paths.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/path_service.h"
#include "brave/components/brave_vpn/v2/app/shared/switches.h"

namespace brave_vpn {
namespace v2 {
namespace {
#if BUILDFLAG(IS_LINUX)
inline constexpr base::FilePath::CharType kBraveCompanyName[] =
    FILE_PATH_LITERAL("brave-software");
#else
inline constexpr base::FilePath::CharType kBraveCompanyName[] =
    FILE_PATH_LITERAL("BraveSoftware");
#endif
}  // namespace

void AppInitLogging(const base::CommandLine& command_line) {
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  base::FilePath log_file_path;
  if (command_line.HasSwitch(switches::kVpnAppLogFile)) {
    settings.logging_dest |= logging::LOG_TO_FILE;
    log_file_path = command_line.GetSwitchValuePath(switches::kVpnAppLogFile);
    settings.log_file_path = log_file_path.value().c_str();
  }
  logging::InitLogging(settings);
}

base::FilePath AppGetPrivilegedUserDataDir(const std::string& product_name) {
  if (product_name.empty()) {
    return base::FilePath();
  }
  base::FilePath result;
#if BUILDFLAG(IS_WIN)
  result = base::PathService::CheckedGet(base::DIR_COMMON_APP_DATA);
#elif BUILDFLAG(IS_MAC)
  result = base::FilePath(FILE_PATH_LITERAL("/Library/Application Support"));
#else
  result = base::FilePath(FILE_PATH_LITERAL("/var/lib"));
#endif
  return result.Append(kBraveCompanyName)
      .Append(base::FilePath::FromUTF8Unsafe(product_name));
}

base::FilePath AppGetUnprivilegedUserDataDir(const std::string& product_name) {
  if (product_name.empty()) {
    return base::FilePath();
  }
  base::FilePath result;
#if BUILDFLAG(IS_WIN)
  result = base::PathService::CheckedGet(base::DIR_LOCAL_APP_DATA);
#else
  result = base::PathService::CheckedGet(base::DIR_APP_DATA);
#endif
  return result.Append(kBraveCompanyName)
      .Append(base::FilePath::FromUTF8Unsafe(product_name));
}

}  // namespace v2
}  // namespace brave_vpn
