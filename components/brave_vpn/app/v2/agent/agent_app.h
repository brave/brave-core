/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_AGENT_APP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_AGENT_APP_H_

namespace brave_vpn {
namespace v2 {

// The AgentApp class encapsulates the main application logic for the Brave VPN
// agent process. It provides a stub method for running the main event loop.

class AgentApp final {
 public:
  AgentApp();
  ~AgentApp();

  AgentApp(const AgentApp&) = delete;
  AgentApp& operator=(const AgentApp&) = delete;

  // Runs the main event loop of the application. The return value is the exit
  // code of the application.
  int Run();
};

}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_AGENT_APP_H_
