/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_CRASH_REPORTER_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_CRASH_REPORTER_CLIENT_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "components/crash/core/app/crash_reporter_client.h"

namespace brave_vpn {
namespace v2 {

class CrashReporterClient final : public crash_reporter::CrashReporterClient {
 public:
  // Initializes the crash reporter client for the current process. This should
  // be called early in the process's initialization, before any crash reporting
  // functionality is used. The parameters are used to configure the crash
  // reporter with the appropriate product name and profile directory for
  // storing crash reports ("Crashpad" subdirectory of the profile directory).
  static void InitializeForProcess(const std::string& process_type,
                                   const std::string& product_name,
                                   const base::FilePath& profile_dir);

#if BUILDFLAG(IS_WIN)
  // On Windows, if the process is a crash handler process, this function will
  // run the crash handler and return the exit code. If the process is not a
  // crash handler process, this function will return std::nullopt. This allows
  // the caller to determine if it should exit immediately after running the
  // crash handler, or if it should continue with normal initialization.
  static std::optional<int> RunAsCrashpadHandlerIfRequired(
      const std::string& process_type,
      const base::FilePath& user_data_dir);
#endif

  ~CrashReporterClient() override;
  CrashReporterClient(const CrashReporterClient&) = delete;
  CrashReporterClient& operator=(const CrashReporterClient&) = delete;

  // crash_reporter::CrashReporterClient override:
  bool IsRunningUnattended() override;
  bool GetCollectStatsConsent() override;
  bool GetCollectStatsInSample() override;
  bool ReportingIsEnforcedByPolicy(bool* breakpad_enabled) override;
#if BUILDFLAG(IS_WIN)
  bool GetShouldDumpLargerDumps() override;
  bool GetCrashDumpLocation(std::wstring* crash_dir) override;
  bool GetCrashMetricsLocation(std::wstring* metrics_dir) override;
  void GetProductNameAndVersion(const std::wstring& exe_path,
                                std::wstring* product_name,
                                std::wstring* version,
                                std::wstring* special_build,
                                std::wstring* channel_name) override;
#else   // BUILDFLAG(IS_WIN)
  bool GetCrashDumpLocation(base::FilePath* crash_dir) override;
  bool GetCrashMetricsLocation(base::FilePath* metrics_dir) override;
#endif  // BUILDFLAG(IS_WIN)

 private:
  // No constuction outside of InitializeForProcess, to ensure the singleton
  // pattern is followed.
  explicit CrashReporterClient(std::string product_name,
                               base::FilePath profile_dir);

  void PlatformInitialize();

  static CrashReporterClient* instance_;

  std::string product_name_;
  base::FilePath profile_dir_;
};

}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_CRASH_REPORTER_CLIENT_H_
