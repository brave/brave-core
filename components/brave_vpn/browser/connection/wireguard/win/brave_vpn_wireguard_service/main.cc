// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "base/win/process_startup_helper.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/brave_wireguard_service_crash_reporter_client.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/install_utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/interactive/interactive_main.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/service_main.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/wireguard_tunnel_service.h"
#include "chrome/install_static/product_install_details.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/crash/core/app/fallback_crash_handling_win.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"

namespace {
const char kUserDataDir[] = "user-data-dir";
const char kProcessType[] = "type";
const char kLogFile[] = "log-file";
}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prev, wchar_t*, int) {
  // Initialize the CommandLine singleton from the environment.
  base::CommandLine::Init(0, nullptr);
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
  if (!process_type.empty()) {
    BraveWireguardCrashReporterClient::InitializeCrashReportingForProcess(
        process_type);
  }
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
  base::win::SetupCRT(*base::CommandLine::ForCurrentProcess());
  install_static::InitializeProductDetailsForPrimaryModule();

  // Initialize COM for the current thread.
  base::win::ScopedCOMInitializer com_initializer(
      base::win::ScopedCOMInitializer::kMTA);
  if (!com_initializer.Succeeded()) {
    PLOG(ERROR) << "Failed to initialize COM";
    return -1;
  }
  if (command_line->HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceConnectSwitchName)) {
    return brave_vpn::wireguard::RunWireguardTunnelService(
        command_line->GetSwitchValuePath(
            brave_vpn::kBraveVpnWireguardServiceConnectSwitchName));
  }
  if (command_line->HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceInteractiveSwitchName)) {
    return brave_vpn::InteractiveMain::GetInstance()->Run();
  }

  // Register and configure windows service.
  if (command_line->HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceInstallSwitchName)) {
    auto success = brave_vpn::InstallBraveWireguardService();
    return success ? 0 : 1;
  }

  if (command_line->HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceUnnstallSwitchName)) {
    auto success = brave_vpn::UninstallBraveWireguardService();
    return success ? 0 : 1;
  }

  // Run the service.
  return brave_vpn::ServiceMain::GetInstance()->Start();
}
