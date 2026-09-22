/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_identity.h"

#include <sys/socket.h>

#include <memory>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/notimplemented.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/platform/platform_handle.h"

namespace brave_vpn::v2 {
namespace {
AgentIdentity::VerificationResult VerifyImpl(base::ProcessId pid,
                                             base::Time creation_time) {
  // The capture is only a pid and a start time, so the first thing verification
  // does is confirm the pid still names the process that connected.
  const base::Time current = base::Process{pid}.CreationTime();
  if (current.is_null() || current != creation_time) {
    // Pid isn't readable or belongs to a different process now.
    return AgentIdentity::VerificationResult::kInconclusive;
  }

  // TODO(https://github.com/brave/brave-browser/issues/54635)
  // Implement the verification step on Linux.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Identity verification is not yet implemented on Linux";
  return AgentIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
std::unique_ptr<AgentIdentity> AgentIdentity::Create(
    const mojo::PlatformChannelEndpoint& endpoint) {
  if (!endpoint.is_valid()) {
    return nullptr;
  }

  struct ucred creds = {};
  socklen_t len = sizeof(creds);
  if (::getsockopt(endpoint.platform_handle().GetFD().get(), SOL_SOCKET,
                   SO_PEERCRED, &creds, &len) != 0) {
    VLOG(1) << "Could not read the agent's credentials from the socket";
    return nullptr;
  }
  if (creds.pid == 0) {
    // The peer is in another pid namespace and the kernel could not translate
    // it, which a retry cannot change (e.g. a sandboxed install).
    VLOG(1) << "Agent pid is not visible in this namespace";
    return nullptr;
  }

  // SO_PEERCRED is taken by the kernel at connect time, so the pid is the
  // process that connected. It can still be recycled between now and
  // verification, which is what the creation time below is for.
  base::Time creation_time = base::Process{creds.pid}.CreationTime();
  if (creation_time.is_null()) {
    VLOG(1) << "Could not read the agent's creation time for pid " << creds.pid;
    return nullptr;
  }
  return base::WrapUnique(new AgentIdentity(creds.pid, creation_time));
}

AgentIdentity::AgentIdentity(base::ProcessId pid, base::Time creation_time)
    : pid_(pid), creation_time_(creation_time) {}

AgentIdentity::VerificationRequestCallback
AgentIdentity::BindVerificationRequest() {
  return base::BindOnce(&VerifyImpl, pid_, creation_time_);
}

}  // namespace brave_vpn::v2
