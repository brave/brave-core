/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_crash_reporter_client.h"

#include <memory>
#include <string>

#include "base/check.h"
#include "base/debug/leak_annotations.h"
#include "base/file_version_info.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "chrome/install_static/user_data_dir.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/version_info/channel.h"

BraveVPNHelperCrashReporterClient::BraveVPNHelperCrashReporterClient() =
    default;

BraveVPNHelperCrashReporterClient::~BraveVPNHelperCrashReporterClient() =
    default;

// static
void BraveVPNHelperCrashReporterClient::InitializeCrashReportingForProcess(
    const std::string& process_type) {
  static BraveVPNHelperCrashReporterClient* instance = nullptr;
  if (instance)
    return;

  instance = new BraveVPNHelperCrashReporterClient();
  ANNOTATE_LEAKING_OBJECT_PTR(instance);
  // Don't set up Crashpad crash reporting in the Crashpad handler itself, nor
  // in the fallback crash handler for the Crashpad handler process.
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    return;
  }
  install_static::InitializeProductDetailsForPrimaryModule();
  crash_reporter::SetCrashReporterClient(instance);
  std::wstring user_data_dir;
  install_static::GetUserDataDirectory(&user_data_dir, nullptr);

  crash_reporter::InitializeCrashpadWithEmbeddedHandler(
      true, brave_vpn::kBraveVPNHelperProcessType,
      install_static::WideToUTF8(user_data_dir), base::FilePath());
}

bool BraveVPNHelperCrashReporterClient::ShouldCreatePipeName(
    const std::wstring& process_type) {
  return false;
}

bool BraveVPNHelperCrashReporterClient::GetAlternativeCrashDumpLocation(
    std::wstring* crash_dir) {
  return false;
}

void BraveVPNHelperCrashReporterClient::GetProductNameAndVersion(
    const std::wstring& exe_path,
    std::wstring* product_name,
    std::wstring* version,
    std::wstring* special_build,
    std::wstring* channel_name) {
  *product_name = brave_vpn::kBraveVpnServiceName;
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

bool BraveVPNHelperCrashReporterClient::ShouldShowRestartDialog(
    std::wstring* title,
    std::wstring* message,
    bool* is_rtl_locale) {
  // There is no UX associated with brave_vpn_helper, so no dialog should be
  // shown.
  return false;
}

bool BraveVPNHelperCrashReporterClient::AboutToRestart() {
  // The brave_vpn_helper should never be restarted after a crash.
  return false;
}

bool BraveVPNHelperCrashReporterClient::GetIsPerUserInstall() {
  return !install_static::IsSystemInstall();
}

bool BraveVPNHelperCrashReporterClient::GetShouldDumpLargerDumps() {
  // Use large dumps for all but the stable channel.
  return install_static::GetChromeChannel() != version_info::Channel::STABLE;
}

int BraveVPNHelperCrashReporterClient::GetResultCodeRespawnFailed() {
  // The restart dialog is never shown.
  NOTREACHED();
  return 0;
}

bool BraveVPNHelperCrashReporterClient::GetCrashDumpLocation(
    std::wstring* crash_dir) {
  *crash_dir = install_static::GetCrashDumpLocation();
  return !crash_dir->empty();
}

bool BraveVPNHelperCrashReporterClient::GetCrashMetricsLocation(
    std::wstring* metrics_dir) {
  install_static::GetUserDataDirectory(metrics_dir, nullptr);
  return !metrics_dir->empty();
}

bool BraveVPNHelperCrashReporterClient::IsRunningUnattended() {
  return true;
}

bool BraveVPNHelperCrashReporterClient::GetCollectStatsConsent() {
  return install_static::GetCollectStatsConsent();
}

bool BraveVPNHelperCrashReporterClient::GetCollectStatsInSample() {
  return install_static::GetCollectStatsInSample();
}

bool BraveVPNHelperCrashReporterClient::ReportingIsEnforcedByPolicy(
    bool* enabled) {
  return install_static::ReportingIsEnforcedByPolicy(enabled);
}

bool BraveVPNHelperCrashReporterClient::ShouldMonitorCrashHandlerExpensively() {
  // The expensive mechanism dedicates a process to be crashpad_handler's own
  // crashpad_handler.
  return false;
}

bool BraveVPNHelperCrashReporterClient::EnableBreakpadForProcess(
    const std::string& process_type) {
  // This is not used by Crashpad (at least on Windows).
  NOTREACHED();
  return true;
}
