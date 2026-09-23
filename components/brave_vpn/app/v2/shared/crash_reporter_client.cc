/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/crash_reporter_client.h"

#include <utility>

#include "base/debug/leak_annotations.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "brave/components/brave_vpn/app/v2/shared/switches.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"

namespace brave_vpn::v2 {

// static
void CrashReporterClient::InitializeForProcess(
    const std::string& process_type,
    const std::string& product_name,
    const std::string& channel_name,
    const base::FilePath& profile_dir) {
  if (instance_) {
    return;
  }
  instance_ = new CrashReporterClient(product_name, channel_name, profile_dir);
  ANNOTATE_LEAKING_OBJECT_PTR(instance_);

#if BUILDFLAG(IS_WIN)
  // Don't set up Crashpad crash reporting in the Crashpad handler itself,
  // nor in the fallback crash handler for the Crashpad handler process.
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    return;
  }
#endif  // BUILDFLAG(IS_WIN)

  crash_reporter::SetCrashReporterClient(instance_);

  DVLOG(1) << "Initializing Crashpad for process type: " << process_type
           << ", product name: " << instance_->product_name_
           << ", channel name: " << instance_->channel_name_
           << ", profile dir: " << instance_->profile_dir_;

  base::File::Error error;
  if (!base::CreateDirectoryAndGetError(instance_->profile_dir_, &error)) {
    LOG(ERROR) << "Failed to create profile dir " << instance_->profile_dir_
               << ": " << base::File::ErrorToString(error);
  }

#if BUILDFLAG(IS_WIN)
  crash_reporter::InitializeCrashpadWithEmbeddedHandler(
      true, process_type, instance_->profile_dir_.AsUTF8Unsafe(),
      base::FilePath());
#else   // BUILDFLAG(IS_WIN)
  crash_reporter::InitializeCrashpad(/*initial_client=*/true, process_type);
#endif  // BUILDFLAG(IS_WIN)
}

CrashReporterClient* CrashReporterClient::instance_ = nullptr;

CrashReporterClient::CrashReporterClient(std::string product_name,
                                         std::string channel_name,
                                         base::FilePath profile_dir)
    : product_name_(std::move(product_name)),
      channel_name_(std::move(channel_name)),
      profile_dir_(std::move(profile_dir)) {}

CrashReporterClient::~CrashReporterClient() = default;

bool CrashReporterClient::IsRunningUnattended() {
  // TODO(https://github.com/brave/brave-browser/issues/56405): return true once
  // we are ready to upload the crash reports.
  return true;
}

bool CrashReporterClient::GetCollectStatsConsent() {
  // TODO(https://github.com/brave/brave-browser/issues/56405): implement this
  // to return the user's consent for collecting crash stats.
  return false;
}

bool CrashReporterClient::GetCollectStatsInSample() {
  // No sampling throttle, collect for everyone using the VPN.
  return true;
}

bool CrashReporterClient::ReportingIsEnforcedByPolicy(
    bool* /*breakpad_enabled*/) {
  // TODO(https://github.com/brave/brave-browser/issues/56405): implement this
  // to return whether configuration management allows loading the crash
  // reporter.
  return false;
}

}  // namespace brave_vpn::v2
