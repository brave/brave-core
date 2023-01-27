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
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_crash_reporter_client.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_state.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/service_main.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/vpn_utils.h"
#include "components/browser_watcher/exit_code_watcher_win.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/crash/core/app/fallback_crash_handling_win.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"

namespace {
const char kUserDataDir[] = "user-data-dir";
const char kProcessType[] = "type";
}  // namespace

int main(int argc, char* argv[]) {
  // Initialize the CommandLine singleton from the environment.
  base::CommandLine::Init(0, nullptr);

  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  logging::InitLogging(settings);

  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(kProcessType);

  BraveVPNHelperCrashReporterClient::InitializeCrashReportingForProcess(
      process_type);
  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    // Check if we should monitor the exit code of this process
    std::unique_ptr<browser_watcher::ExitCodeWatcher> exit_code_watcher;

    crash_reporter::SetupFallbackCrashHandling(*command_line);
    // The handler process must always be passed the user data dir on the
    // command line.
    DCHECK(command_line->HasSwitch(kUserDataDir));

    base::FilePath user_data_dir =
        command_line->GetSwitchValuePath(kUserDataDir);
    int crashpad_status = crash_reporter::RunAsCrashpadHandler(
        *base::CommandLine::ForCurrentProcess(), user_data_dir, kProcessType,
        kUserDataDir);
    if (crashpad_status != 0 && exit_code_watcher) {
      // Crashpad failed to initialize, explicitly stop the exit code watcher
      // so the crashpad-handler process can exit with an error
      exit_code_watcher->StopWatching();
    }
    return crashpad_status;
  }

  // Make sure the process exits cleanly on unexpected errors.
  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
  base::win::RegisterInvalidParamHandler();

  base::win::SetupCRT(*command_line);

  // Register vpn helper service in the system.
  if (command_line->HasSwitch(brave_vpn::kBraveVpnHelperInstall)) {
    auto success = brave_vpn::ConfigureServiceAutoRestart(
        brave_vpn::GetVpnServiceName(), brave_vpn::GetBraveVPNConnectionName());
    return success ? 0 : 1;
  }

  // Run the service.
  brave_vpn::ServiceMain* service = brave_vpn::ServiceMain::GetInstance();
  if (!service->InitWithCommandLine(command_line)) {
    return 1;
  }

  return service->Start();
}
