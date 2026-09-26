/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_identity.h"

#include <windows.h>

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/notimplemented.h"
#include "base/process/process.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/platform/platform_handle.h"

namespace brave_vpn::v2 {
namespace {
AgentIdentity::VerificationResult VerifyImpl(base::Process process) {
  if (!process.IsValid()) {
    return AgentIdentity::VerificationResult::kInconclusive;
  }

  // TODO(https://github.com/brave/brave-browser/issues/54633)
  // Implement the verification step on Windows.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Identity verification is not yet implemented on Windows";
  return AgentIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
std::unique_ptr<AgentIdentity> AgentIdentity::Create(
    const mojo::PlatformChannelEndpoint& endpoint) {
  if (!endpoint.is_valid()) {
    return nullptr;
  }

  ULONG server_pid = 0;
  if (!::GetNamedPipeServerProcessId(
          endpoint.platform_handle().GetHandle().get(), &server_pid)) {
    VLOG(1) << "Could not read the agent's pid from the pipe";
    return nullptr;
  }

  // Pin the process: a pid on its own is not an identity, and holding a handle
  // is what stops it being recycled under us between here and verification.
  base::Process process =
      base::Process::OpenWithAccess(static_cast<base::ProcessId>(server_pid),
                                    PROCESS_QUERY_LIMITED_INFORMATION);
  if (!process.IsValid()) {
    VLOG(1) << "Could not open the agent process " << server_pid;
    return nullptr;
  }
  return base::WrapUnique(new AgentIdentity(std::move(process)));
}

AgentIdentity::AgentIdentity(base::Process process)
    : process_(std::move(process)) {}

AgentIdentity::VerificationRequestCallback
AgentIdentity::BindVerificationRequest() {
  return base::BindOnce(&VerifyImpl, std::move(process_));
}

}  // namespace brave_vpn::v2
