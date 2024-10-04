// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/process_startup_helper.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/brave_wireguard_service_crash_reporter_client.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/notifications/notification_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_service_runner.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_tunnel_service.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/status_tray_runner.h"
#include "brave/browser/brave_vpn/win/service_constants.h"
#include "chrome/install_static/product_install_details.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/crashpad.h"
#include "components/crash/core/app/fallback_crash_handling_win.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
constexpr char kUserDataDir[] = "user-data-dir";
constexpr char kProcessType[] = "type";
constexpr char kLogFile[] = "log-file";
}  // namespace

// List of commands executed on user level and interacting with users.
std::optional<int> ProcessUserLevelCommands(
    const base::CommandLine& command_line) {
  brave_vpn::LoadLocaleResources();
  // User level command line. In this mode creates an invisible window and sets
  // an icon in the status tray to interact with the user. The icon shows a
  // pop-up menu to control the connection of the Wireguard VPN without
  // interacting with the browser.
  if (command_line.HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceInteractiveSwitchName)) {
    return brave_vpn::StatusTrayRunner::GetInstance()->Run();
  }

  // User level command line. Publishes notification to system notification
  // center when vpn connected.
  if (command_line.HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceNotifyConnectedSwitchName)) {
    brave_vpn::ShowDesktopNotification(
        base::UTF16ToWide(l10n_util::GetStringUTF16(
            IDS_BRAVE_VPN_WIREGUARD_TRAY_NOTIFICATION_CONNECTED)));
    return 0;
  }

  // User level command line. Publishes notification to system notification
  // center when vpn disconnected.
  if (command_line.HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceNotifyDisconnectedSwitchName)) {
    brave_vpn::ShowDesktopNotification(
        base::UTF16ToWide(l10n_util::GetStringUTF16(
            IDS_BRAVE_VPN_WIREGUARD_TRAY_NOTIFICATION_DISCONNECTED)));
    return 0;
  }
  return std::nullopt;
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prev, wchar_t*, int) {
  // Process Mitigation Redirection Policy
  PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY signature = {0};
  DWORD dwSize = sizeof(signature);
  signature.EnforceRedirectionTrust = 1;
  SetProcessMitigationPolicy(ProcessRedirectionTrustPolicy, &signature, dwSize);

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

  // System level command line. In this mode, loads the tunnel.dll and passes it
  // the path to the config. all control of the service is given to tunnel.dll,
  // stops when execution returns.
  if (command_line->HasSwitch(
          brave_vpn::kBraveVpnWireguardServiceConnectSwitchName)) {
    return brave_vpn::wireguard::RunWireguardTunnelService(
        command_line->GetSwitchValuePath(
            brave_vpn::kBraveVpnWireguardServiceConnectSwitchName));
  }

  auto result = ProcessUserLevelCommands(*command_line);
  if (result.has_value()) {
    return result.value();
  }
  // Runs BraveVpnWireguardService, called by system SCM.
  return brave_vpn::WireguardServiceRunner::GetInstance()->RunAsService();
}
