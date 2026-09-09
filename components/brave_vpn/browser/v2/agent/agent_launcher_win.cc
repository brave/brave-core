/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/notimplemented.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"

namespace brave_vpn::v2::internal {

base::FilePath GetValidAgentPath() {
  // TODO(https://github.com/brave/brave-browser/issues/58243)
  // Implement agent path building and validation logic for Windows.
  // Empty path means there is no valid agent executable found.
  NOTIMPLEMENTED_LOG_ONCE()
      << "GetValidAgentPath() is not yet implemented on Windows";
  return {};
}

void LaunchAgentProcess(AgentLauncher::LaunchFailureCallback failure_callback,
                        const base::FilePath& agent_path) {
  // TODO(https://github.com/brave/brave-browser/issues/58243)
  // Implement agent launch logic for Windows using appropriate Windows APIs.
  NOTIMPLEMENTED_LOG_ONCE()
      << "LaunchAgentProcess() is not yet implemented on Windows";
}

}  // namespace brave_vpn::v2::internal
