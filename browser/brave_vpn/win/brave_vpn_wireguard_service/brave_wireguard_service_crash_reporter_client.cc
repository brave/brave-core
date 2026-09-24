/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/brave_wireguard_service_crash_reporter_client.h"

#include <memory>
#include <string>

#include "base/debug/leak_annotations.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "chrome/install_static/user_data_dir.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/version_info/channel.h"

namespace {
// Split into two places to avoid patching:
// chromium_src\components\crash\core\app\crashpad.cc
// Need keep it in sync.
constexpr char kBraveWireguardProcessType[] = "brave-vpn-wireguard-service";

// The service starts under sytem user so we save crashes to
// %PROGRAMDATA%\BraveSoftware\{service name}\Crashpad
base::FilePath GetBraveWireguardServiceProfileDir() {
  auto program_data = install_static::GetEnvironmentString("PROGRAMDATA");
  if (program_data.empty()) {
    return base::FilePath();
  }
  return base::FilePath(base::UTF8ToWide(program_data))
      .Append(install_static::kCompanyPathName)
      .Append(brave_vpn::GetBraveVpnWireguardServiceName());
}

}  // namespace

BraveWireguardCrashReporterClient::BraveWireguardCrashReporterClient() =
    default;

BraveWireguardCrashReporterClient::~BraveWireguardCrashReporterClient() =
    default;

// static
void BraveWireguardCrashReporterClient::InitializeCrashReportingForProcess(
    const std::string& process_type) {
  static BraveWireguardCrashReporterClient* instance = nullptr;
  if (instance) {
    return;
  }

  instance = new BraveWireguardCrashReporterClient();
  ANNOTATE_LEAKING_OBJECT_PTR(instance);
  // Don't set up Crashpad crash reporting in the Crashpad handler itself, nor
  // in the fallback crash handler for the Crashpad handler process.
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    return;
  }
  install_static::InitializeProductDetailsForPrimaryModule();
  crash_reporter::SetCrashReporterClient(instance);

  crash_reporter::InitializeCrashpadWithEmbeddedHandler(
      true, kBraveWireguardProcessType,
      install_static::WideToUTF8(GetBraveWireguardServiceProfileDir().value()),
      base::FilePath());
}

bool BraveWireguardCrashReporterClient::GetAlternativeCrashDumpLocation(
    std::wstring* crash_dir) {
  return false;
}

void BraveWireguardCrashReporterClient::GetProductNameAndVersion(
    const std::wstring& exe_path,
    std::wstring* product_name,
    std::wstring* version,
    std::wstring* special_build,
    std::wstring* channel_name) {
  *product_name = brave_vpn::GetBraveVpnWireguardServiceName();
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

bool BraveWireguardCrashReporterClient::GetShouldDumpLargerDumps() {
  // Use large dumps for all but the stable channel.
  return install_static::GetChromeChannel() != version_info::Channel::STABLE;
}

bool BraveWireguardCrashReporterClient::GetCrashDumpLocation(
    std::wstring* crash_dir) {
  auto profile_dir = GetBraveWireguardServiceProfileDir();
  *crash_dir = (profile_dir.Append(L"Crashpad")).value();
  return !profile_dir.empty();
}

bool BraveWireguardCrashReporterClient::GetCrashMetricsLocation(
    std::wstring* metrics_dir) {
  *metrics_dir = GetBraveWireguardServiceProfileDir().value();
  return !metrics_dir->empty();
}

bool BraveWireguardCrashReporterClient::IsRunningUnattended() {
  return false;
}

bool BraveWireguardCrashReporterClient::GetCollectStatsConsent() {
  return install_static::GetCollectStatsConsent();
}

bool BraveWireguardCrashReporterClient::GetCollectStatsInSample() {
  return install_static::GetCollectStatsInSample();
}

bool BraveWireguardCrashReporterClient::ReportingIsEnforcedByPolicy(
    bool* enabled) {
  return install_static::ReportingIsEnforcedByPolicy(enabled);
}

bool BraveWireguardCrashReporterClient::ShouldMonitorCrashHandlerExpensively() {
  // The expensive mechanism dedicates a process to be crashpad_handler's own
  // crashpad_handler.
  return false;
}

bool BraveWireguardCrashReporterClient::EnableBreakpadForProcess(
    const std::string& process_type) {
  // This is not used by Crashpad (at least on Windows).
  NOTREACHED();
}
