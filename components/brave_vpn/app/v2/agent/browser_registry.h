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
#include "mojo/public/cpp/platform/platform_handle.h"
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
// including a reference to that same accept-time capture. An entry is created
// during browser initialization call and lives until its connection goes away,
// so it outlives the |peers_| entry and lets a browser re-bind a host after the
// capture expires. Values are heap-allocated because authentication and
// verification functions work with pointers into the map.
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

  // Enum describing whether a connection's peer has been verified.
  enum class ConnectionState {
    // Resolved against an accept-time capture, with verification in flight or
    // about to start. Also where a connection whose verification was refused
    // stays: nothing on this connection can move it on from here.
    kIdentified,
    // Verified. May bind a host, and may bind another one after dropping the
    // first: the verdict belongs to the connection, not to the session.
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

  // Everything InitializeBrowser() must carry across the verification hop,
  // since none of it can be re-read from dispatch state afterwards.
  struct PendingInit {
    mojo::ReceiverId receiver_id = 0;
    mojo::PlatformHandle identity_channel;
    base::OnceCallback<void(mojom::InitializeResult)> reply;
  };

  // Wires up and starts the host server; shared by both constructors.
  void StartHostServer();

  // BrowserHostProviderImpl::Delegate:
  void InitializeBrowser(
      uint32_t protocol_version,
      mojo::PlatformHandle identity_channel,
      base::OnceCallback<void(mojom::InitializeResult)> callback) override;

  void BindBrowserHost(
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      base::OnceCallback<void(mojom::BindBrowserHostResult)> callback) override;

  // Accept-time policy: cheap, non-blocking checks only; nullptr refuses the
  // connection.
  brave_vpn::mojom::BrowserHostProvider* OnBrowserConnecting(
      const named_mojo_ipc_server::ConnectionInfo& info);

  // Returns the accept-time capture belonging to the connection described by
  // |info|, or null if there is none to verify against. Called from
  // InitializeBrowser(), before any connection entry exists. Drops the capture
  // it looked at when that capture has expired or belongs to another process.
  scoped_refptr<BrowserIdentity> ResolvePeer(
      const named_mojo_ipc_server::ConnectionInfo& info);

  // Second half of InitializeBrowser(): marks the connection verified and
  // answers the identity channel.
  void OnPeerVerified(PendingInit pending,
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
