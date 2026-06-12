/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/process/memory.h"
#include "brave/components/brave_vpn/app/v2/helper/helper_app.h"
#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"
#include "brave/components/brave_vpn/app/v2/shared/crash_reporter_client.h"
#include "brave/components/brave_vpn/app/v2/shared/switches.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/process_startup_helper.h"
#endif

using brave_vpn::v2::CrashReporterClient;
using brave_vpn::v2::HelperApp;

namespace {
constexpr char kBraveVpnHelperAppProcessType[] = "brave-vpn-helper-app";
constexpr char kBraveVpnHelperAppProductName[] = "BraveVpnHelperApp";
}  // namespace

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
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
#endif  // BUILDFLAG(IS_WIN)

  brave_vpn::v2::app_utils::InitLogging(command_line);
#if BUILDFLAG(IS_MAC)
  brave_vpn::v2::app_utils::ConfigureFrameworkBundlePath();
#endif

  if (process_type.empty()) {
    process_type = kBraveVpnHelperAppProcessType;
  }
  std::string channel_name = command_line.GetSwitchValueASCII(
      brave_vpn::v2::switches::kVpnAppChannelName);
  if (channel_name.empty()) {
    channel_name = "unknown";
  }
  CrashReporterClient::InitializeForProcess(
      process_type, kBraveVpnHelperAppProductName, channel_name,
      brave_vpn::v2::app_utils::GetUserDataDir(kBraveVpnHelperAppProductName,
                                               /*is_privileged_process=*/true));

  HelperApp helper_app;
  return helper_app.Run();
}
