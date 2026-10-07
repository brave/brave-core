/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_registry.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/process_handle.h"
#include "base/sequence_checker.h"
#include "base/strings/strcat.h"
#include "base/task/bind_post_task.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_host_impl.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"
#include "build/build_config.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/win_util.h"
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_MAC)
#include "brave/components/brave_vpn/common/v2/identity_channel_mac.h"
#endif  // BUILDFLAG(IS_MAC)

namespace brave_vpn::v2 {
namespace {
// Oldest contract the agent still speaks; mojom::kProtocolVersion is the
// newest. Bumped only when support for older browsers is deliberately dropped,
// which makes every such bump a compatibility break worth reviewing.
constexpr uint32_t kMinSupportedProtocolVersion = 1;

// How long an accepted connection may stay silent before the agent stops
// holding the peer reference it captured. Prevents a connection that never
// calls BindBrowserHost() from pinning a process handle for the life of the
// agent.
constexpr base::TimeDelta kCapturedPeerIdleTimeout = base::Seconds(10);

named_mojo_ipc_server::EndpointOptions GetEndpointOptions(
    mojo::NamedPlatformChannel::ServerName server_name) {
  named_mojo_ipc_server::EndpointOptions endpoint_options(
      std::move(server_name),
      named_mojo_ipc_server::EndpointOptions::kUseIsolatedConnection);
#if BUILDFLAG(IS_WIN)
  std::wstring user_sid;
  CHECK(base::win::GetUserSidString(&user_sid));
  endpoint_options.security_descriptor =
      base::StrCat({L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;", user_sid, L")"});
  endpoint_options.include_peer_process_info = true;
#endif  // BUILDFLAG(IS_WIN)
  return endpoint_options;
}

bool IsCapturedPeerExpired(const base::ElapsedTimer& time_since_capture) {
  return time_since_capture.Elapsed() > kCapturedPeerIdleTimeout;
}
}  // namespace

BrowserRegistry::BrowserRegistry(
    mojo::NamedPlatformChannel::ServerName server_name)
    : host_server_(std::make_unique<named_mojo_ipc_server::NamedMojoIpcServer<
                       brave_vpn::mojom::BrowserHostProvider>>(
          GetEndpointOptions(std::move(server_name)),
          base::BindRepeating(&BrowserRegistry::OnBrowserConnecting,
                              base::Unretained(this)))) {
  StartHostServer();
}

BrowserRegistry::BrowserRegistry(
    std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server)
    : host_server_(std::move(host_server)) {
  StartHostServer();
}

BrowserRegistry::~BrowserRegistry() = default;

// static
std::unique_ptr<BrowserRegistry> BrowserRegistry::CreateForTesting(  // IN-TEST
    std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server) {
  return base::WrapUnique(new BrowserRegistry(std::move(host_server)));
}

void BrowserRegistry::StartHostServer() {
  VLOG(1) << "Starting IPC server";
  host_server_->set_disconnect_handler(base::BindRepeating(
      &BrowserRegistry::OnHostProviderDisconnected, base::Unretained(this)));
  host_server_->StartServer();
}

brave_vpn::mojom::BrowserHostProvider* BrowserRegistry::OnBrowserConnecting(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RemoveExpiredPeers();

  // Runs synchronously on the IPC sequence, so nothing blocking belongs here.
  // The only jobs are to reject peers that are cheap to rule out, and to pin
  // the peer so the expensive check later inspects the process that actually
  // connected rather than whatever holds its pid by then.
  scoped_refptr<BrowserIdentity> identity = BrowserIdentity::Create(info);
  if (!identity) {
    return nullptr;
  }

  const base::ProcessId pid = identity->pid();
  if (auto it = peers_.find(pid); it != peers_.end()) {
    if (it->second.identity->IsSameProcess(*identity)) {
      // The process already has a live capture; sibling profile connections
      // share it, and the timeout stays measured from the first one.
      VLOG(1) << "New browser connecting: "
              << it->second.identity->GetDescription();
      return &host_provider_;
    }
    // Same pid, different process: the previous occupant exited and the pid was
    // reused. Any connection still relying on the old entry must not be
    // verified against this new process.
    VLOG(1) << "Dropping stale accept-time identity for reused pid " << pid;
    peers_.erase(it);
  }

  VLOG(1) << "New browser connecting: " << identity->GetDescription();
  peers_.emplace(pid, Peer{.identity = std::move(identity)});
  return &host_provider_;
}

scoped_refptr<BrowserIdentity> BrowserRegistry::ResolvePeer(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Capturing the dispatching peer is how its pid is read; this identity is
  // discarded, since the accept-time one is what pins the process.
  scoped_refptr<BrowserIdentity> dispatching = BrowserIdentity::Create(info);
  if (!dispatching) {
    return nullptr;
  }

  // Must be on the accept-time entry list.
  auto connecting = peers_.find(dispatching->pid());
  if (connecting == peers_.end()) {
    // The capture is gone: it aged out and was swept, or this connection was
    // never accepted; there is nothing to verify against.
    return nullptr;
  }

  // Check for expiry so a capture is never usable past its deadline no matter
  // when storage is actually reclaimed.
  if (IsCapturedPeerExpired(connecting->second.time_since_capture)) {
    peers_.erase(connecting);
    return nullptr;
  }

  // Same pid is not the same process. A peer whose connection outlives its own
  // capture cannot be allowed to resolve against a capture taken for whoever
  // inherited its pid afterwards. Refusing here is not a verdict about this
  // peer, so it reads as retryable.
  scoped_refptr<BrowserIdentity> identity = connecting->second.identity;
  if (!identity->IsSameProcess(*dispatching)) {
    VLOG(1) << "Refusing " << dispatching->GetDescription()
            << ": capture held for pid belongs to another process ("
            << identity->GetDescription() << ")";
    peers_.erase(connecting);
    return nullptr;
  }

  // The accept-time entry is deliberately left in place. A browser process
  // holds one connection per profile, and at launch those may arrive together:
  // all of them are captured under the same pid before any of them dispatches.
  // Consuming the entry here would resolve the first profile and leave every
  // other one unresolvable, which reads to the browser as rejected and takes
  // those profiles to "unavailable" state for the life of the process.
  return identity;
}

void BrowserRegistry::InitializeBrowser(
    uint32_t protocol_version,
    mojo::PlatformHandle identity_channel,
    base::OnceCallback<void(mojom::InitializeResult)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Every initialization reply is posted, never run inline.
  auto reply = base::BindPostTaskToCurrentDefault(std::move(callback));

  // Called synchronously from Initialize(), so the calling browser is still the
  // current receiver.
  const mojo::ReceiverId receiver_id = host_server_->current_receiver();

  if (connections_.contains(receiver_id)) {
    // One successful Initialize() per connection. Nothing about a connection's
    // peer can change while it lives, so a second call is a bug or not our
    // browser.
    VLOG(1) << "Refusing browser " << receiver_id << ": already initialized";
    std::move(reply).Run(mojom::InitializeResult::kInvalidRequest);
    return;
  }

  // Every version in [kMinSupportedProtocolVersion, kProtocolVersion] is
  // accepted, so a browser and agent from different updates still interoperate.
  // A browser newer than this agent is refused, because it would go on to call
  // methods this build does not implement. This deliberately leaves no
  // connection entry behind: browser may try again with a version agent speaks.
  if (protocol_version < kMinSupportedProtocolVersion ||
      protocol_version > mojom::kProtocolVersion) {
    VLOG(1) << "Refusing browser " << receiver_id << ": protocol version "
            << protocol_version << " outside supported range ["
            << kMinSupportedProtocolVersion << ", " << mojom::kProtocolVersion
            << "]";
    std::move(reply).Run(mojom::InitializeResult::kVersionMismatch);
    return;
  }

  // Resolve the peer now: the call arrives one round trip after the connection
  // was accepted, so the capture is all but certain to still be live.
  scoped_refptr<BrowserIdentity> identity =
      ResolvePeer(host_server_->current_connection_info());
  if (!identity) {
    VLOG(1) << "Refusing browser " << receiver_id
            << ": no accept-time identity for the connection (expires "
            << kCapturedPeerIdleTimeout << " after connect)";
    std::move(reply).Run(mojom::InitializeResult::kNotIdentified);
    return;
  }

  // The entry exists from here on, before the verification hop rather than
  // after it. That is what bounds this connection to one verification: a second
  // Initialize() finds the entry above and is refused, so a peer cannot make
  // the agent verify repeatedly by asking again.
  auto connection = std::make_unique<Connection>();
  connection->identity = identity;
  connections_.emplace(receiver_id, std::move(connection));

  PendingInit pending{.receiver_id = receiver_id,
                      .identity_channel = std::move(identity_channel),
                      .reply = std::move(reply)};

  // Verification may block, so it is BrowserIdentity's task to keep the
  // blocking part off this sequence.
  identity->Verify(base::BindOnce(&BrowserRegistry::OnPeerVerified,
                                  weak_factory_.GetWeakPtr(),
                                  std::move(pending)));
}

void BrowserRegistry::BindBrowserHost(
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host,
    base::OnceCallback<void(mojom::BindBrowserHostResult)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Every reply is posted, never run inline.
  auto reply = base::BindPostTaskToCurrentDefault(std::move(callback));

  // Called synchronously from BindBrowserHost(), so the calling browser is
  // still the current receiver.
  const mojo::ReceiverId receiver_id = host_server_->current_receiver();

  Connection* connection = base::FindPtrOrNull(connections_, receiver_id);

  // The verdict is what gates this, not the mere existence of an entry: a
  // connection whose verification is still in flight, or was refused, has an
  // entry and must not get a host.
  if (!connection || connection->state != ConnectionState::kVerified) {
    VLOG(1) << "Refusing browser " << receiver_id << ": not initialized";
    std::move(reply).Run(mojom::BindBrowserHostResult::kUninitialized);
    return;
  }

  if (connection->host_session) {
    // One BrowserHost per connection. A browser that drops its host may ask
    // again, and is not re-verified: the verdict belongs to the connection.
    VLOG(1) << "Refusing browser " << receiver_id << ": host already bound";
    std::move(reply).Run(mojom::BindBrowserHostResult::kAlreadyBound);
    return;
  }

  connection->host_session = std::make_unique<BrowserHostImpl>(
      std::move(browser_endpoint), std::move(host),
      base::BindOnce(&BrowserRegistry::OnHostDisconnected,
                     weak_factory_.GetWeakPtr(), receiver_id));

  VLOG(1) << "Browser " << receiver_id
          << " bound host: " << connection->identity->GetDescription();
  std::move(reply).Run(mojom::BindBrowserHostResult::kSuccess);
}

void BrowserRegistry::OnPeerVerified(
    PendingInit pending,
    BrowserIdentity::VerificationResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const mojo::ReceiverId receiver_id = pending.receiver_id;

  // The connection may have gone away mid-verification; its entry is the only
  // thing that still says whether it is around, and it is erased by
  // OnHostProviderDisconnected().
  auto* connection = base::FindPtrOrNull(connections_, receiver_id);
  if (!connection) {
    VLOG(1) << "Browser " << receiver_id << " went away during verification";
    // The pipe is already closed, so this reply goes nowhere; run it anyway
    // rather than dropping a response callback. The identity channel goes out
    // of scope unanswered, which is what a peer that left should get.
    std::move(pending.reply).Run(mojom::InitializeResult::kInconclusive);
    return;
  }

  DCHECK_EQ(ConnectionState::kIdentified, connection->state);
  DCHECK(!connection->host_session);

  switch (result) {
    case BrowserIdentity::VerificationResult::kAccepted:
      break;
    case BrowserIdentity::VerificationResult::kRejected:
      // A verdict about this binary, which does not change while it runs. The
      // entry stays so a second Initialize() is refused rather than verifying
      // the same peer again.
      VLOG(1) << "Browser " << receiver_id << " failed verification: "
              << connection->identity->GetDescription();
      std::move(pending.reply).Run(mojom::InitializeResult::kRejected);
      return;
    case BrowserIdentity::VerificationResult::kInconclusive:
      VLOG(1) << "Browser " << receiver_id << " could not be verified: "
              << connection->identity->GetDescription();
      std::move(pending.reply).Run(mojom::InitializeResult::kInconclusive);
      return;
  }

#if BUILDFLAG(IS_MAC)
  // Deliberately after the verdict: the agent self-identifies only to a browser
  // it has accepted, so a peer that fails verification learns nothing about who
  // is serving it. Also before the reply, so the browser usually finds the
  // message already queued rather than depending on this process still being
  // scheduled.
  SendIdentityMessage(std::move(pending.identity_channel));
#endif  // BUILDFLAG(IS_MAC)

  connection->state = ConnectionState::kVerified;

  // Nothing is removed from |peers_| here on purpose: sibling profiles of this
  // same browser process may still be waiting to dispatch, and they resolve
  // through that entry.
  VLOG(1) << "Browser " << receiver_id
          << " initialized: " << connection->identity->GetDescription();
  std::move(pending.reply).Run(mojom::InitializeResult::kSuccess);
}

void BrowserRegistry::RemoveExpiredPeers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const size_t reclaimed = absl::erase_if(peers_, [](const auto& entry) {
    return IsCapturedPeerExpired(entry.second.time_since_capture);
  });
  if (reclaimed) {
    VLOG(1) << "Removed " << reclaimed
            << " accept-time captures (expired after "
            << kCapturedPeerIdleTimeout << ")";
  }
}

void BrowserRegistry::OnHostProviderDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Runs while the disconnecting receiver is still current; the server closes
  // it itself once this returns.
  const mojo::ReceiverId receiver_id = host_server_->current_receiver();
  VLOG(1) << "Browser " << receiver_id << " disconnected";

  // Per the mojom contract, losing the connection tears down its BrowserHost
  // too, and abandons any verification still in flight.
  connections_.erase(receiver_id);
}

void BrowserRegistry::OnHostDisconnected(mojo::ReceiverId id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(1) << "Browser " << id << " dropped a session pipe";

  auto* connection = base::FindPtrOrNull(connections_, id);
  if (connection) {
    // The connection stays up, keeps its identity and its verdict, so the
    // browser may bind another host without being verified again; only the
    // session goes away.
    connection->host_session.reset();
  }
}

}  // namespace brave_vpn::v2
