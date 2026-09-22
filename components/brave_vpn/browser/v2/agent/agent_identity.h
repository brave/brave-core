/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_IDENTITY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_IDENTITY_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/platform/platform_handle.h"

namespace brave_vpn::v2 {

// AgentIdentity is a reference to the process serving one agent connection,
// taken while that connection is being established. All member functions are
// cheap and non-blocking, and the expensive part of verification is posted to a
// thread pool and reported via a callback.
//
// How the peer is pinned differs by platform, which is why capture happens on
// the connecting sequence while the transport endpoint is still intact:
//  - Windows and Linux: read the server's process from the endpoint, before
//    mojo consumes it to accept the invitation.
//  - macOS: there is no way to ask who holds a Mach port's receive right, and a
//    peer's audit token only ever arrives stamped by the kernel on a message.
//    So the client creates an identity channel here, hands the send right to
//    the agent in Initialize(), and reads the token off the reply.
class AgentIdentity {
 public:
  enum class VerificationResult {
    // Verified as Brave VPN agent.
    kAccepted,
    // Verified as NOT a Brave VPN agent: a verdict about the binary, which does
    // not change while it runs.
    kRejected,
    // Could not tell: image replaced mid-update, cert rotated, no identity
    // message arrived. A later attempt may succeed, so this must
    // never be treated as a rejection.
    kInconclusive,
  };

  using VerificationRequestCallback = base::OnceCallback<VerificationResult()>;
  using VerificationResponseCallback =
      base::OnceCallback<void(VerificationResult)>;

  // Captures the identity of the server reachable through |endpoint|, or
  // returns null if the peer cannot be pinned. Runs on the connecting sequence,
  // before mojo consumes |endpoint|; may block depending on a platform.
  static std::unique_ptr<AgentIdentity> Create(
      const mojo::PlatformChannelEndpoint& endpoint);

  virtual ~AgentIdentity();

  AgentIdentity(const AgentIdentity&) = delete;
  AgentIdentity& operator=(const AgentIdentity&) = delete;

  // The send channel handle to hand the agent in Initialize(), on platforms
  // where the agent has to speak before it can be identified at all. An invalid
  // handle everywhere else, which tells the agent this client is not asking to
  // verify it. May only be called once.
  mojo::PlatformHandle TakeSendHandle();

  // Posts an expensive part of verification to a thread pool with all the
  // platform-specific arguments it needs, and calls |callback| with the result.
  // |callback| is never invoked synchronously, and always runs on the sequence
  // that called Verify(), which must therefore have a current default task
  // runner. May only be called once: on macOS the verification consumes the
  // identity channel, which carries a single message.
  virtual void Verify(VerificationResponseCallback callback);

 protected:
#if BUILDFLAG(IS_WIN)
  explicit AgentIdentity(base::Process process);
#elif BUILDFLAG(IS_LINUX)
  AgentIdentity(base::ProcessId pid, base::Time creation_time);
#elif BUILDFLAG(IS_MAC)
  AgentIdentity(mojo::PlatformHandle receive_handle,
                mojo::PlatformHandle send_handle);
#endif  // BUILDFLAG(IS_MAC)

  // Returns a closure carrying copies of the platform data the blocking
  // verification needs, so it can run without touching the object. Consumes
  // what it takes, so this may only be called once.
  VerificationRequestCallback BindVerificationRequest();

#if BUILDFLAG(IS_WIN)
  base::Process process_;
#elif BUILDFLAG(IS_LINUX)
  const base::ProcessId pid_;
  const base::Time creation_time_;
#elif BUILDFLAG(IS_MAC)
  mojo::PlatformHandle receive_handle_;
#endif  // BUILDFLAG(IS_MAC)
  mojo::PlatformHandle send_handle_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_AGENT_IDENTITY_H_
