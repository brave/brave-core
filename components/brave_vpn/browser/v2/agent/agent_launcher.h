/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_H_

#include <string_view>

#include "base/functional/callback.h"

namespace brave_vpn::v2 {
// AgentLauncher starts the VPN agent process.
//
// Fire-and-forget by design. The launcher does not check whether the agent is
// already running, does not verify the binary, does not wait for the agent to
// come up, and does not report success. The callback exists purely so the
// caller learns that process creation itself failed, and can stop waiting.
//
// AgentLauncher instances are cheap and are owned per profile. Two profiles
// launching concurrently is harmless: the VPN agent process enforces its own
// singleton, and a duplicate agent instance exits quietly.
class AgentLauncher {
 public:
  enum class LaunchError {
    // The agent app was not found where it was expected to be.
    kAppNotFound,
    // Process creation was attempted and did not happen.
    kLaunchFailed,
  };
  using LaunchFailureCallback = base::OnceCallback<void(LaunchError)>;

  static std::string_view ErrorToString(LaunchError error);

  AgentLauncher() = default;
  virtual ~AgentLauncher();

  AgentLauncher(const AgentLauncher&) = delete;
  AgentLauncher& operator=(const AgentLauncher&) = delete;

  // Creates a detached agent process. Returns immediately; the work happens on
  // a blocking sequence. Safe to call more than once, though callers are not
  // expected to need it.
  virtual void Launch(LaunchFailureCallback failure_callback);
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_H_
