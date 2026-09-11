/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <windows.h>

#include <optional>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/win/access_token.h"
#include "brave/components/brave_vpn/app/v2/agent/branding_buildflags.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"

namespace brave_vpn::v2::internal {
namespace {

// The filtered, medium integrity token of an elevated browser; empty when the
// browser is not elevated, or when the session has no lower integrity level to
// drop to.
std::optional<base::win::AccessToken> GetDeElevatedToken() {
  std::optional<base::win::AccessToken> token =
      base::win::AccessToken::FromCurrentProcess(
          /*impersonation=*/false, TOKEN_DUPLICATE | TOKEN_QUERY);
  if (!token || !token->IsElevated()) {
    return std::nullopt;
  }

  std::optional<base::win::AccessToken> filtered_token = token->LinkedToken();
  if (!filtered_token) {
    return std::nullopt;
  }
  return filtered_token->DuplicatePrimary(TOKEN_QUERY | TOKEN_DUPLICATE |
                                          TOKEN_ASSIGN_PRIMARY);
}

bool LaunchDetached(const base::CommandLine& command_line, HANDLE as_user) {
  base::LaunchOptions options;
  options.as_user = as_user;
  options.start_hidden = true;

  // The browser may itself sit inside a job object with "kill on close",
  // which would take the agent down with it. Try to breakaway first, though
  // it may fail due to lack of the necessary job object permissions.
  options.force_breakaway_from_job_ = true;
  if (base::LaunchProcess(command_line, options).IsValid()) {
    return true;
  }
  options.force_breakaway_from_job_ = false;
  return base::LaunchProcess(command_line, options).IsValid();
}

void CreateAgentProcess(AgentLauncher::LaunchFailureCallback failure_callback,
                        const base::FilePath& agent_path) {
  // The agent needs no elevated privileges, so it must not get them just
  // because the browser happens to be running elevated. Launching with the
  // filtered token puts it at the integrity level the rest of the session uses.
  const std::optional<base::win::AccessToken> token = GetDeElevatedToken();
  if (!LaunchDetached(base::CommandLine(agent_path),
                      token ? token->get() : nullptr)) {
    std::move(failure_callback).Run(AgentLauncher::LaunchError::kLaunchFailed);
  }
}

}  // namespace

base::FilePath GetValidAgentPath() {
  const base::FilePath agent_path =
      base::PathService::CheckedGet(base::DIR_EXE)
          .AppendASCII(BUILDFLAG(VPN_AGENT_EXE_NAME));
  if (!base::PathExists(agent_path)) {
    LOG(ERROR) << "VPN agent not found at " << agent_path;
    return {};
  }
  return agent_path;
}

void LaunchAgentProcess(AgentLauncher::LaunchFailureCallback failure_callback,
                        const base::FilePath& agent_path) {
  // Process creation blocks, so it cannot run on the calling (UI) sequence. The
  // failure callback is already bound to the caller's sequence, so the task can
  // run it directly.
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&CreateAgentProcess, std::move(failure_callback),
                     agent_path));
}

}  // namespace brave_vpn::v2::internal
