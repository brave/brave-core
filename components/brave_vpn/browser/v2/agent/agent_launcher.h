/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_H_

#include <memory>
#include <string_view>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"

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
//
// Only the most recent launch can report. Starting a launch supersedes any
// launch still in flight: the policy is "latest wins". The first launch
// establishes the launcher's sequence, and all subsequent launches and failure
// callbacks are expected to run on it.
class AgentLauncher {
 public:
  enum class LaunchError {
    // The agent app was not found where it was expected to be.
    kAppNotFound,
    // Process creation was attempted and did not happen.
    kLaunchFailed,
  };
  using LaunchFailureCallback = base::OnceCallback<void(LaunchError)>;

  // Resolves the agent's path; runs on a blocking sequence, so an override
  // must not reach back into state owned by the launcher's sequence.
  using AgentPathResolver = base::RepeatingCallback<base::FilePath()>;

  // Creates the agent process, reporting failure (and only failure) through
  // the callback; runs on the launcher's sequence and must not block.
  using ProcessLauncher = base::RepeatingCallback<void(LaunchFailureCallback,
                                                       const base::FilePath&)>;

  static std::string_view ErrorToString(LaunchError error);

  AgentLauncher();
  virtual ~AgentLauncher();

  AgentLauncher(const AgentLauncher&) = delete;
  AgentLauncher& operator=(const AgentLauncher&) = delete;

  // The test seam replaces the path lookup and the process creation, so the
  // superseding logic can be exercised on every platform without a real agent
  // binary and without creating processes.
  static std::unique_ptr<AgentLauncher> CreateForTesting(
      AgentPathResolver path_resolver,
      ProcessLauncher process_launcher);

  // Creates a detached agent process. Returns immediately; the work happens on
  // a blocking sequence. Safe to call more than once, though previous calls are
  // superseded, and failure callbacks from earlier launches may not run.
  virtual void Launch(LaunchFailureCallback failure_callback);

 private:
  AgentLauncher(AgentPathResolver path_resolver,
                ProcessLauncher process_launcher);

  void OnAgentPathResolved(LaunchFailureCallback failure_callback,
                           base::FilePath agent_path);
  void OnLaunchFailed(LaunchFailureCallback failure_callback,
                      LaunchError error);

  SEQUENCE_CHECKER(sequence_checker_);
  const AgentPathResolver path_resolver_;
  const ProcessLauncher process_launcher_;

  // Invalidated by Launch(); nothing unrelated to a launch may be bound to it.
  base::WeakPtrFactory<AgentLauncher> launch_weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_LAUNCHER_H_
