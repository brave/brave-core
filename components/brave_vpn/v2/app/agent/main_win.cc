/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/process/memory.h"
#include "base/win/process_startup_helper.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/v2/app/agent/agent_app.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prev, wchar_t*, int) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(0, nullptr);
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();

  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(command_line);

  brave_vpn::v2::AgentApp agent_app;
  auto init_result = agent_app.Initialize(command_line);
  if (init_result.has_value()) {
    return init_result.value();
  }
  return agent_app.Run();
}
