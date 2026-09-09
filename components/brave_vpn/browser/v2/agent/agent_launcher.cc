/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher.h"

#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/task/bind_post_task.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"

namespace brave_vpn::v2 {
namespace {
void LaunchIfFound(AgentLauncher::LaunchFailureCallback failure_callback,
                   const base::FilePath& agent_path) {
  if (agent_path.empty()) {
    std::move(failure_callback).Run(AgentLauncher::LaunchError::kAppNotFound);
    return;
  }
  internal::LaunchAgentProcess(
      base::BindPostTaskToCurrentDefault(std::move(failure_callback)),
      agent_path);
}
}  // namespace

// static
std::string_view AgentLauncher::ErrorToString(LaunchError error) {
  switch (error) {
    case LaunchError::kAppNotFound:
      return "agent not found";
    case LaunchError::kLaunchFailed:
      return "launch failed";
  }
}

AgentLauncher::~AgentLauncher() = default;

void AgentLauncher::Launch(LaunchFailureCallback failure_callback) {
  // Get the agent path on a blocking sequence, then launch the agent process on
  // the UI sequence. A platform-specific LaunchAgentProcess() will decide
  // whether it needs to post another blocking task to do the actual launch. The
  // reply runs on the calling sequence.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&internal::GetValidAgentPath),
      base::BindOnce(&LaunchIfFound, std::move(failure_callback)));
}

}  // namespace brave_vpn::v2
