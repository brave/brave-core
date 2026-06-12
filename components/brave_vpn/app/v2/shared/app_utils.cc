/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"

#include "base/base_paths.h"
#include "base/check.h"
#include "base/environment.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/path_service.h"
#include "brave/components/brave_vpn/app/v2/shared/switches.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_LINUX)
#include "base/nix/xdg_util.h"
#endif

namespace brave_vpn::v2::app_utils {
namespace {
inline constexpr base::FilePath::CharType kBraveCompanyName[] =
    FILE_PATH_LITERAL("BraveSoftware");
}  // namespace

void InitLogging(const base::CommandLine& command_line) {
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

base::FilePath GetUserDataDir(const std::string& product_name,
                              bool is_privileged_process) {
  CHECK(!product_name.empty());
  base::FilePath result;
#if BUILDFLAG(IS_WIN)
  result = base::PathService::CheckedGet(is_privileged_process
                                             ? base::DIR_COMMON_APP_DATA
                                             : base::DIR_LOCAL_APP_DATA);
#elif BUILDFLAG(IS_MAC)
  result =
      is_privileged_process
          ? base::FilePath(FILE_PATH_LITERAL("/Library/Application Support"))
          : base::PathService::CheckedGet(base::DIR_APP_DATA);
#elif BUILDFLAG(IS_LINUX)
  if (is_privileged_process) {
    result = base::FilePath(FILE_PATH_LITERAL("/var/lib"));
  } else {
    auto env = base::Environment::Create();
    result = base::nix::GetXDGDirectory(
        env.get(), base::nix::kXdgConfigHomeEnvVar, base::nix::kDotConfigDir);
  }
#else
#error unsupported platform
#endif
  return result.Append(kBraveCompanyName)
      .Append(base::FilePath::FromUTF8Unsafe(product_name));
}

}  // namespace brave_vpn::v2::app_utils
