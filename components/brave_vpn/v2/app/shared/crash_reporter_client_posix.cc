/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/v2/app/shared/crash_reporter_client.h"

#include "base/files/file_path.h"

namespace brave_vpn {
namespace v2 {

bool CrashReporterClient::GetCollectStatsConsent() {
  return false;
}

bool CrashReporterClient::GetCollectStatsInSample() {
  return true;
}

bool CrashReporterClient::ReportingIsEnforcedByPolicy(
    bool* /*breakpad_enabled*/) {
  return false;
}

bool CrashReporterClient::GetCrashDumpLocation(base::FilePath* crash_dir) {
  *crash_dir = profile_dir_.Append(FILE_PATH_LITERAL("Crashpad"));
  return !profile_dir_.empty();
}

bool CrashReporterClient::GetCrashMetricsLocation(base::FilePath* metrics_dir) {
  *metrics_dir = profile_dir_;
  return !profile_dir_.empty();
}

void CrashReporterClient::PlatformInitialize() {}

}  // namespace v2
}  // namespace brave_vpn
