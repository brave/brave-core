/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_identity.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/notimplemented.h"
#include "brave/components/brave_vpn/common/v2/identity_channel.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"

namespace brave_vpn::v2 {
namespace {
AgentIdentity::VerificationResult VerifyImpl(
    mojo::PlatformHandle receive_handle) {
  const std::optional<audit_token_t> audit_token =
      ReadIdentityMessage(std::move(receive_handle));
  if (!audit_token) {
    // No message, or not the message this contract describes. Not a verdict
    // about the peer: a later connection may well get one.
    return AgentIdentity::VerificationResult::kInconclusive;
  }

  // TODO(https://github.com/brave/brave-browser/issues/54634)
  // Implement the verification step on macOS.
  NOTIMPLEMENTED_LOG_ONCE()
      << "Identity verification is not yet implemented on macOS";
  return AgentIdentity::VerificationResult::kAccepted;
}
}  // namespace

// static
std::unique_ptr<AgentIdentity> AgentIdentity::Create(
    const mojo::PlatformChannelEndpoint& /*endpoint*/) {
  // |endpoint| is deliberately unused: on macOS the peer of a Mach port cannot
  // be queried, so the identity comes from a message the agent sends on a
  // channel created here rather than from the transport itself.
  std::optional<IdentityChannel> channel = CreateIdentityChannel();
  if (!channel) {
    return nullptr;
  }
  return base::WrapUnique(new AgentIdentity(std::move(channel->receive_handle),
                                            std::move(channel->send_handle)));
}

AgentIdentity::AgentIdentity(mojo::PlatformHandle receive_handle,
                             mojo::PlatformHandle send_handle)
    : receive_handle_(std::move(receive_handle)),
      send_handle_(std::move(send_handle)) {}

AgentIdentity::VerificationRequestCallback
AgentIdentity::BindVerificationRequest() {
  return base::BindOnce(&VerifyImpl, std::move(receive_handle_));
}

}  // namespace brave_vpn::v2
