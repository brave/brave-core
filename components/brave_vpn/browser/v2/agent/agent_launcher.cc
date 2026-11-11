/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/sequence_checker.h"
#include "base/task/bind_post_task.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"

namespace brave_vpn::v2 {
// static
std::string_view AgentLauncher::ErrorToString(LaunchError error) {
  switch (error) {
    case LaunchError::kAppNotFound:
      return "agent not found";
    case LaunchError::kLaunchFailed:
      return "launch failed";
  }
}

AgentLauncher::AgentLauncher()
    : AgentLauncher(base::BindRepeating(&internal::GetValidAgentPath),
                    base::BindRepeating(&internal::LaunchAgentProcess)) {}

AgentLauncher::AgentLauncher(AgentPathResolver path_resolver,
                             ProcessLauncher process_launcher)
    : path_resolver_(std::move(path_resolver)),
      process_launcher_(std::move(process_launcher)) {
  CHECK(path_resolver_);
  CHECK(process_launcher_);
}

AgentLauncher::~AgentLauncher() = default;

// static
std::unique_ptr<AgentLauncher> AgentLauncher::CreateForTesting(  // IN-TEST
    AgentPathResolver path_resolver,
    ProcessLauncher process_launcher) {
  return base::WrapUnique(
      new AgentLauncher(std::move(path_resolver), std::move(process_launcher)));
}

void AgentLauncher::Launch(LaunchFailureCallback failure_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Supersede any launch still in flight: its failure, arriving later, would
  // say nothing about this one.
  launch_weak_factory_.InvalidateWeakPtrs();

  // Get the agent path on a blocking sequence.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(path_resolver_),
      base::BindOnce(&AgentLauncher::OnAgentPathResolved,
                     launch_weak_factory_.GetWeakPtr(),
                     std::move(failure_callback)));
}

void AgentLauncher::OnAgentPathResolved(LaunchFailureCallback failure_callback,
                                        base::FilePath agent_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (agent_path.empty()) {
    std::move(failure_callback).Run(LaunchError::kAppNotFound);
    return;
  }
  // Launch the agent process on the UI sequence. A platform-specific
  // LaunchAgentProcess() will decide whether it needs to post another blocking
  // task to do the actual launch. The reply runs on the calling sequence.
  process_launcher_.Run(
      base::BindPostTaskToCurrentDefault(base::BindOnce(
          &AgentLauncher::OnLaunchFailed, launch_weak_factory_.GetWeakPtr(),
          std::move(failure_callback))),
      agent_path);
}

void AgentLauncher::OnLaunchFailed(LaunchFailureCallback failure_callback,
                                   LaunchError error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // This hop exists to route the reply through a weak pointer, so a failure
  // from a superseded launch is dropped instead of reported.
  std::move(failure_callback).Run(error);
}

}  // namespace brave_vpn::v2
