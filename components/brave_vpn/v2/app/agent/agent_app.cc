/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/v2/app/agent/agent_app.h"

#include "base/check.h"
#include "base/immediate_crash.h"
#include "base/logging.h"
#include "brave/components/brave_vpn/v2/app/shared/app_utils.h"
#include "brave/components/brave_vpn/v2/app/shared/crash_reporter_client.h"
#include "brave/components/brave_vpn/v2/app/shared/switches.h"
#include "build/build_config.h"

namespace brave_vpn {
namespace v2 {

namespace {
// Split into two places to avoid patching:
// chromium_src/components/crash/core/app/crashpad.cc
// Need keep it in sync
constexpr char kBraveVpnAgentAppProcessType[] = "brave-vpn-agent-app";

constexpr char kBraveVpnAgentAppProductName[] = "BraveVpnAgentApp";
}  // namespace

AgentApp::AgentApp() : initialized_(false) {}

AgentApp::~AgentApp() = default;

std::optional<int> AgentApp::Initialize(const base::CommandLine& command_line) {
  CHECK(!initialized_) << "Initialize should only be called once.";
  AppInitLogging(command_line);

  auto process_type =
      command_line.GetSwitchValueASCII(switches::kVpnAppProcessType);
  if (process_type.empty()) {
    process_type = kBraveVpnAgentAppProcessType;
  }
#if BUILDFLAG(IS_WIN)
  auto crashpad_handler_status =
      CrashReporterClient::RunAsCrashpadHandlerIfRequired(
          process_type,
          command_line.GetSwitchValuePath(switches::kVpnAppUserDataDir));
  if (crashpad_handler_status.has_value()) {
    return crashpad_handler_status;
  }
#endif  // BUILDFLAG(IS_WIN)
  CrashReporterClient::InitializeForProcess(
      process_type, kBraveVpnAgentAppProductName,
      AppGetUnprivilegedUserDataDir(kBraveVpnAgentAppProductName));

  if (command_line.HasSwitch(switches::kVpnAppCrashOnStartup)) {
    base::ImmediateCrash();
  }
  initialized_ = true;
  return std::nullopt;
}

int AgentApp::Run() {
  CHECK(initialized_) << "Run should only be called after initialization.";
  VLOG(1) << "Hello from the Brave VPN Agent!";
  return 0;
}

}  // namespace v2
}  // namespace brave_vpn
