/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_INTERNAL_H_

#include "base/files/file_path.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher.h"

namespace brave_vpn::v2::internal {

// Absolute path of the agent executable (Windows, Linux) or app bundle (macOS),
// derived from the browser's own install location. Returns an empty path if the
// install layout is unrecognized, or the agent executable cannot be found.
base::FilePath GetValidAgentPath();

// Creates a detached agent process. The new process must survive this browser
// exiting, must not inherit its handles or standard streams, and must not be
// visible to the user. Called on the UI sequence, so this must not block. The
// blocking work is posted if the platform API is synchronous. Failure callback
// runs exactly once on failure; it is already bound to the caller's sequence,
// so it may be run, or dropped, from any sequence.
void LaunchAgentProcess(AgentLauncher::LaunchFailureCallback failure_callback,
                        const base::FilePath& agent_path);

}  // namespace brave_vpn::v2::internal

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_INTERNAL_H_
