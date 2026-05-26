/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/v2/app/shared/crash_reporter_client.h"

#include "base/debug/leak_annotations.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "brave/components/brave_vpn/v2/app/shared/switches.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"

namespace brave_vpn {
namespace v2 {

// static
void CrashReporterClient::InitializeForProcess(
    const std::string& process_type,
    const std::string& product_name,
    const base::FilePath& profile_dir) {
  if (instance_) {
    return;
  }
  instance_ = new CrashReporterClient(product_name, profile_dir);
  ANNOTATE_LEAKING_OBJECT_PTR(instance_);

  // Don't set up Crashpad crash reporting in the Crashpad handler itself,
  // nor in the fallback crash handler for the Crashpad handler process.
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    return;
  }

  instance_->PlatformInitialize();
  crash_reporter::SetCrashReporterClient(instance_);

  VLOG(1) << "Initializing Crashpad for process type: " << process_type
          << ", product name: " << product_name
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
  crash_reporter::InitializeCrashpad(true, process_type);
#endif  // BUILDFLAG(IS_WIN)
}

CrashReporterClient* CrashReporterClient::instance_ = nullptr;

CrashReporterClient::CrashReporterClient(std::string product_name,
                                         base::FilePath profile_dir)
    : product_name_(std::move(product_name)),
      profile_dir_(std::move(profile_dir)) {}

CrashReporterClient::~CrashReporterClient() = default;

bool CrashReporterClient::IsRunningUnattended() {
  // Never upload crash reports.
  return true;
}

}  // namespace v2
}  // namespace brave_vpn
