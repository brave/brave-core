// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <windows.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/process_startup_helper.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_crash_reporter_client.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/service_main.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/vpn_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/crash/core/app/fallback_crash_handling_win.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"

namespace {
constexpr char kUserDataDir[] = "user-data-dir";
constexpr char kProcessType[] = "type";
constexpr char kLogFile[] = "log-file";
}  // namespace

int main(int argc, char* argv[]) {
  // Process Mitigation Redirection Policy
  PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY signature = {0};
  DWORD dwSize = sizeof(signature);
  signature.EnforceRedirectionTrust = 1;
  SetProcessMitigationPolicy(ProcessRedirectionTrustPolicy, &signature, dwSize);

  // Initialize the CommandLine singleton from the environment.
  base::CommandLine::Init(argc, argv);
  auto* command_line = base::CommandLine::ForCurrentProcess();
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  base::FilePath log_file_path;
  if (command_line->HasSwitch(kLogFile)) {
    settings.logging_dest |= logging::LOG_TO_FILE;
    log_file_path = command_line->GetSwitchValuePath(kLogFile);
    settings.log_file_path = log_file_path.value().c_str();
  }
  logging::InitLogging(settings);
  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;
  std::string process_type = command_line->GetSwitchValueASCII(kProcessType);

  BraveVPNHelperCrashReporterClient::InitializeCrashReportingForProcess(
      process_type);
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    crash_reporter::SetupFallbackCrashHandling(*command_line);
    // The handler process must always be passed the user data dir on the
    // command line.
    DCHECK(command_line->HasSwitch(kUserDataDir));

    base::FilePath user_data_dir =
        command_line->GetSwitchValuePath(kUserDataDir);
    int crashpad_status = crash_reporter::RunAsCrashpadHandler(
        *base::CommandLine::ForCurrentProcess(), user_data_dir, kProcessType,
        kUserDataDir);
    return crashpad_status;
  }

  // Make sure the process exits cleanly on unexpected errors.
  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
  base::win::RegisterInvalidParamHandler();

  base::win::SetupCRT(*command_line);

  // Run the service.
  brave_vpn::ServiceMain* service = brave_vpn::ServiceMain::GetInstance();
  if (!service->InitWithCommandLine(command_line)) {
    return 1;
  }

  return service->Start();
}
