/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/process/memory.h"
#include "brave/components/brave_vpn/app/v2/agent/agent_app.h"
#include "brave/components/brave_vpn/app/v2/agent/single_instance.h"
#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"
#include "brave/components/brave_vpn/app/v2/shared/crash_reporter_client.h"
#include "brave/components/brave_vpn/app/v2/shared/switches.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/process_startup_helper.h"
#include "base/win/windows_types.h"
#endif

using brave_vpn::v2::AgentApp;
using brave_vpn::v2::CrashReporterClient;
using brave_vpn::v2::SingleInstance;

namespace {
// Split into two places to avoid patching:
// chromium_src/components/crash/core/app/crashpad.cc
// Need keep it in sync.
constexpr char kBraveVpnAgentAppProcessType[] = "brave-vpn-agent-app";

constexpr char kBraveVpnAgentAppProductName[] = "BraveVpnAgentApp";
}  // namespace

#if BUILDFLAG(IS_WIN)
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
#else
int main(int argc, char* argv[]) {
#endif
  base::AtExitManager exit_manager;
#if BUILDFLAG(IS_WIN)
  base::CommandLine::Init(0, nullptr);
#else
  base::CommandLine::Init(argc, argv);
#endif
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
#if BUILDFLAG(IS_WIN)
  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(command_line);
#endif

  std::string process_type = command_line.GetSwitchValueASCII(
      brave_vpn::v2::switches::kVpnAppProcessType);
#if BUILDFLAG(IS_WIN)
  auto crashpad_handler_status =
      CrashReporterClient::RunAsCrashpadHandlerIfRequired(
          process_type, command_line.GetSwitchValuePath(
                            brave_vpn::v2::switches::kVpnAppUserDataDir));
  if (crashpad_handler_status.has_value()) {
    return crashpad_handler_status.value();
  }

  // Attach a console to the process if requested.
  if (command_line.HasSwitch(brave_vpn::v2::switches::kVpnAppConsole)) {
    base::RouteStdioToConsole(/*create_console_if_not_found=*/true);
  }
#endif  // BUILDFLAG(IS_WIN)

  brave_vpn::v2::app_utils::InitLogging(command_line);
#if BUILDFLAG(IS_MAC)
  brave_vpn::v2::app_utils::ConfigureFrameworkBundlePath();
#endif

  // Create the user data directory for the agent process.
  base::FilePath user_data_dir = brave_vpn::v2::app_utils::GetUserDataDir(
      kBraveVpnAgentAppProductName, /*is_privileged_process=*/false);
  if (!base::CreateDirectory(user_data_dir)) {
    LOG(ERROR) << "Failed to create user-data dir " << user_data_dir;
    return 1;
  }

  // Only one instance of the agent is allowed to run per OS session. Try to
  // acquire the single-instance slot. If it fails, exit cleanly.
  auto single_instance = SingleInstance::TryAcquire(user_data_dir);
  if (!single_instance) {
    return 0;
  }

  if (process_type.empty()) {
    process_type = kBraveVpnAgentAppProcessType;
  }
  std::string channel_name = command_line.GetSwitchValueASCII(
      brave_vpn::v2::switches::kVpnAppChannelName);
  if (channel_name.empty()) {
    channel_name = "unknown";
  }
  CrashReporterClient::InitializeForProcess(
      process_type, kBraveVpnAgentAppProductName, channel_name, user_data_dir);

  AgentApp agent_app;
  return agent_app.Run();
}
