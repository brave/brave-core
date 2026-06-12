/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/crash_reporter_client.h"

#include <memory>

#include "base/command_line.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/app/v2/shared/switches.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/crash/core/app/fallback_crash_handling_win.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"
#include "components/version_info/channel.h"

namespace brave_vpn {
namespace v2 {

// static
std::optional<int> CrashReporterClient::RunAsCrashpadHandlerIfRequired(
    const std::string& process_type,
    const base::FilePath& user_data_dir) {
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    const auto& command_line = *base::CommandLine::ForCurrentProcess();
    crash_reporter::SetupFallbackCrashHandling(command_line);
    return crash_reporter::RunAsCrashpadHandler(command_line, user_data_dir,
                                                switches::kVpnAppProcessType,
                                                switches::kVpnAppUserDataDir);
  }
  return std::nullopt;
}

bool CrashReporterClient::GetCollectStatsConsent() {
  return install_static::GetCollectStatsConsent();
}

bool CrashReporterClient::GetCollectStatsInSample() {
  return install_static::GetCollectStatsInSample();
}

bool CrashReporterClient::ReportingIsEnforcedByPolicy(bool* breakpad_enabled) {
  return install_static::ReportingIsEnforcedByPolicy(breakpad_enabled);
}

bool CrashReporterClient::GetShouldDumpLargerDumps() {
  // Use large dumps for all but the stable channel.
  return install_static::GetChromeChannel() != version_info::Channel::STABLE;
}

bool CrashReporterClient::GetCrashDumpLocation(std::wstring* crash_dir) {
  if (profile_dir_.empty()) {
    return false;
  }
  *crash_dir = profile_dir_.Append(L"Crashpad").value();
  return true;
}

bool CrashReporterClient::GetCrashMetricsLocation(std::wstring* metrics_dir) {
  if (profile_dir_.empty()) {
    return false;
  }
  *metrics_dir = profile_dir_.value();
  return true;
}

void CrashReporterClient::GetProductNameAndVersion(const std::wstring& exe_path,
                                                   std::wstring* product_name,
                                                   std::wstring* version,
                                                   std::wstring* special_build,
                                                   std::wstring* channel_name) {
  *product_name = base::UTF8ToWide(product_name_);
  std::unique_ptr<FileVersionInfo> version_info(
      FileVersionInfo::CreateFileVersionInfo(base::FilePath(exe_path)));
  if (version_info) {
    *version = base::AsWString(version_info->product_version());
    *special_build = base::AsWString(version_info->special_build());
  } else {
    *version = L"0.0.0.0-devel";
    *special_build = std::wstring();
  }
  *channel_name =
      install_static::GetChromeChannelName(/*with_extended_stable=*/true);
}

void CrashReporterClient::PlatformInitialize() {
  install_static::InitializeProductDetailsForPrimaryModule();
}

}  // namespace v2
}  // namespace brave_vpn
