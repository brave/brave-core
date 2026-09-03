/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_REGISTRY_H_

#include <stdint.h>

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process_handle.h"
#include "base/sequence_checker.h"
#include "base/timer/elapsed_timer.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_host_provider_impl.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "components/named_mojo_ipc_server/ipc_server.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace brave_vpn::v2 {

class BrowserHostImpl;

// Owns the IPC server and every browser to agent connection on it, plus the
// authentication policy the unauthenticated surface delegates to. Sessions
// are keyed by the BrowserHostProvider receiver id, which identifies the
// connection: one authenticated BrowserHost per connection at a time.
//
// Authentication state is split across two maps:
//
// |peers_| is keyed by pid and holds the accept-time capture of each connecting
// browser process. The capture pins the process, so verification inspects the
// process that connected rather than whatever holds its pid later. Entries are
// dropped on timeout, on a reused pid, or on a mismatch at dispatch. They are
// never dropped on success, because sibling profile connections from the same
// process resolve through them.
//
// |connections_| is keyed by receiver id and holds per-connection state,
// including a reference to that same accept-time capture. A capture taken at
// dispatch is not used: only pid() and IsSameProcess() are meaningful on one.
// An entry lives until its connection goes away, so it outlives the |peers_|
// entry and lets a browser re-bind a host after the capture expires. Values are
// heap-allocated because ResolveConnection() returns a pointer into the map.
class BrowserRegistry : public BrowserHostProviderImpl::Delegate {
 public:
  explicit BrowserRegistry(mojo::NamedPlatformChannel::ServerName server_name);
  ~BrowserRegistry() override;

  BrowserRegistry(const BrowserRegistry&) = delete;
  BrowserRegistry& operator=(const BrowserRegistry&) = delete;

  // Takes the IPC server directly, so a test can choose connection ids and
  // report disconnects without a real endpoint.
  static std::unique_ptr<BrowserRegistry> CreateForTesting(
      std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server);

 private:
  friend class BrowserRegistryTest;

  explicit BrowserRegistry(
      std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server);

  // Enum describing where a connection is in the authentication sequence.
  enum class ConnectionState {
    // Resolved against an accept-time capture, but with no host bound and no
    // verification running: either it has not authenticated yet, or a previous
    // attempt was refused, or its host pipe was dropped. May authenticate.
    kIdentified,
    // A verification is in flight.
    kVerifying,
    // Verified, with a live BrowserHostImpl.
    kVerified,
  };

  // Per-connection state, keyed by BrowserHostProvider receiver id.
  struct Connection {
    ConnectionState state = ConnectionState::kIdentified;
    scoped_refptr<BrowserIdentity> identity;
    std::unique_ptr<BrowserHostImpl> host_session;
  };

  // A peer captured at accept time. Keyed by pid, so two connections from one
  // process share an identity.
  struct Peer {
    scoped_refptr<BrowserIdentity> identity;
    base::ElapsedTimer time_since_capture;
  };

  // Everything Authenticate() must carry across the verification hop, since
  // none of it can be re-read from dispatch state afterwards.
  struct PendingAuth {
    mojo::ReceiverId receiver_id = 0;
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint;
    mojo::PendingReceiver<mojom::BrowserHost> host;
    base::OnceCallback<void(mojom::BrowserAuthResult)> reply;
  };

  // Wires up and starts the host server; shared by both constructors.
  void StartHostServer();

  // BrowserHostProviderImpl::Delegate:
  void Authenticate(
      uint32_t protocol_version,
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      base::OnceCallback<void(mojom::BrowserAuthResult)> callback) override;

  // Accept-time policy: cheap, non-blocking checks only; nullptr refuses the
  // connection.
  brave_vpn::mojom::BrowserHostProvider* OnBrowserConnecting(
      const named_mojo_ipc_server::ConnectionInfo& info);

  // Null if the connection is not known.
  Connection* FindConnection(mojo::ReceiverId receiver_id);

  // Returns the entry for the connection currently dispatching, creating it
  // from the accept-time capture on first use so a browser that drops its host
  // and asks again resolves to the same peer. Null if the connection was never
  // captured or its accept-time entry already timed out.
  Connection* ResolveConnection(mojo::ReceiverId receiver_id);

  // Second half of Authenticate(): creates the browser host on success.
  void OnPeerVerified(PendingAuth pending,
                      BrowserIdentity::VerificationResult result);

  // Drops expired entries, releasing the process references they pin.
  void RemoveExpiredPeers();

  void OnHostProviderDisconnected();
  void OnHostDisconnected(mojo::ReceiverId id);

  // Shared by every connection: the IPC server hands the same instance to each
  // one, so it must outlive the host server.
  BrowserHostProviderImpl host_provider_{this};

  std::unique_ptr<named_mojo_ipc_server::IpcServer> host_server_;
  absl::flat_hash_map<base::ProcessId, Peer> peers_;
  absl::flat_hash_map<mojo::ReceiverId, std::unique_ptr<Connection>>
      connections_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<BrowserRegistry> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_BROWSER_REGISTRY_H_
