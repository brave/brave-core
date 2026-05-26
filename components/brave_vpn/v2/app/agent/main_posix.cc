/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/process/memory.h"
#include "brave/components/brave_vpn/v2/app/agent/agent_app.h"

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();

  brave_vpn::v2::AgentApp agent_app;
  auto init_result = agent_app.Initialize(command_line);
  if (init_result.has_value()) {
    return init_result.value();
  }
  return agent_app.Run();
}
