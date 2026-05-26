/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_AGENT_AGENT_APP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_AGENT_AGENT_APP_H_

#include <optional>

#include "base/command_line.h"

namespace brave_vpn {
namespace v2 {

// The AgentApp class encapsulates the main application logic for the Brave VPN
// agent process. It provides methods for initializing the application based on
// command line arguments and running the main event loop.

class AgentApp final {
 public:
  AgentApp();
  ~AgentApp();

  AgentApp(const AgentApp&) = delete;
  AgentApp& operator=(const AgentApp&) = delete;

  // Initialize method returns an optional int, which can be used to
  // indicate an early exit code if initialization fails or if the application
  // should exit immediately (e.g., after running a crash handler). If
  // initialization is successful and the application should continue running,
  // it returns std::nullopt.
  std::optional<int> Initialize(const base::CommandLine& command_line);

  // Runs the main event loop of the application. This should only be called
  // after successful initialization. The return value is the exit code of the
  // application.
  int Run();

 private:
  bool initialized_;
};

}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_AGENT_AGENT_APP_H_
