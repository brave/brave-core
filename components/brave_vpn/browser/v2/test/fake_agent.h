/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_TEST_FAKE_AGENT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_TEST_FAKE_AGENT_H_

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <optional>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "brave/components/brave_vpn/browser/v2/agent_client.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_vpn::v2 {
// FakeAgent simulates agent's IPC server, in-process. It substitutes for the
// whole transport: the connector hands the client one end of an ordinary
// message pipe instead of connecting a channel and accepting an invitation.
// Nothing here needs a rendezvous or a mojo IPC thread. Everything past the
// pipe - the handshake, refusals, retries, teardown - runs the real code on
// real mojo pipes; the transport itself is the part this does not cover. The
// caller should construct this on the sequence the client will live on and call
// everything from there, except the two knobs marked as read off-sequence.
class FakeAgent : public brave_vpn::mojom::BrowserHostProvider {
 public:
  FakeAgent();
  ~FakeAgent() override;

  FakeAgent(const FakeAgent&) = delete;
  FakeAgent& operator=(const FakeAgent&) = delete;

  // Callbacks to pass to AgentClient::CreateForTesting().
  AgentClient::ServerNameProvider GetServerNameProvider();
  AgentClient::Connector GetConnector();

  // Whether a server name can be resolved at all; "false" models a session the
  // agent cannot be reached in on any attempt which the client treats as
  // terminal rather than retryable. Read off-sequence.
  void set_server_name_available(bool available) {
    server_name_available_.store(available);
  }

  // Whether the transport can be established; "false" models an agent that is
  // not running, which the client retries. Read off-sequence.
  void set_transport_fails(bool fails) { transport_fails_.store(fails); }

  // The reply to BindBrowserHost(), or nullopt to withhold it: an agent that
  // accepts the connection and then goes quiet. Defaults to kAccepted.
  void set_auth_result(std::optional<mojom::BrowserAuthResult> result);

  // Sends a reply withheld by set_auth_result(std::nullopt). Lets a test order
  // the reply against other events instead of racing two pipes: dropping the
  // session handles first and answering kAccepted after is what reproduces an
  // acceptance landing on a session that is already gone.
  void AnswerHeldRequest(mojom::BrowserAuthResult result);

  // Closes the BrowserEndpoint remotes and BrowserHost receivers handed over so
  // far, leaving the connections themselves up. Models the agent dropping a
  // session without exiting.
  void DropSessionHandles();

  // Closes everything: connections, sessions, and any withheld reply. Models
  // the agent process exiting.
  void CloseAllConnections();

  // Attempts to establish the transport, including ones failed by the knobs
  // above. Updated off-sequence.
  int connect_attempts() const { return connect_attempts_.load(); }

  // Calls to BindBrowserHost() across all connections.
  int bind_browser_host_calls() const;

  // The version the last BindBrowserHost() carried, or nullopt if it has not
  // been called.
  std::optional<uint32_t> last_protocol_version() const;

  // Connections currently bound: a number of clients the agent could still
  // answer.
  size_t connection_count() const;

  // Sessions currently held: a number of accepted handshakes whose handles are
  // alive.
  size_t session_count() const;

  // Whether the agent has a way to call back into a browser, as opposed to only
  // a host request.
  bool has_browser_endpoint() const { return session_count() > 0u; }

  bool has_held_request() const;

 private:
  // Both of these run on a blocking sequence, like their production
  // counterparts, so they touch only the atomics and what they create.
  std::optional<mojo::NamedPlatformChannel::ServerName> GetServerName();
  mojo::ScopedMessagePipeHandle Connect(
      scoped_refptr<base::SequencedTaskRunner> agent_task_runner,
      const mojo::NamedPlatformChannel::ServerName& server_name);

  // Binds the provider to the agent's end of the pipe, back on the agent's own
  // sequence, since that is where mojo objects have to live.
  void BindProvider(mojo::ScopedMessagePipeHandle pipe);

  // brave_vpn::mojom::BrowserHostProvider:
  void BindBrowserHost(
      uint32_t protocol_version,
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      BindBrowserHostCallback callback) override;

  // Keeps the handles a BindBrowserHost() handed over. BrowserHost has no
  // methods yet, so its receiver is parked rather than bound: holding the pipe
  // open is all the client can observe.
  void KeepSession(mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
                   mojo::PendingReceiver<mojom::BrowserHost> host);

  std::atomic<bool> server_name_available_{true};
  std::atomic<bool> transport_fails_{false};
  std::atomic<int> connect_attempts_{0};

  std::optional<mojom::BrowserAuthResult> auth_result_ GUARDED_BY_CONTEXT(
      sequence_checker_) = mojom::BrowserAuthResult::kAccepted;
  std::optional<uint32_t> last_protocol_version_
      GUARDED_BY_CONTEXT(sequence_checker_);
  int bind_browser_host_calls_ GUARDED_BY_CONTEXT(sequence_checker_) = 0;
  BindBrowserHostCallback held_reply_ GUARDED_BY_CONTEXT(sequence_checker_);

  mojo::ReceiverSet<mojom::BrowserHostProvider> provider_receivers_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::vector<mojo::Remote<mojom::BrowserEndpoint>> browser_endpoints_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::vector<mojo::PendingReceiver<mojom::BrowserHost>> parked_hosts_
      GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_TEST_FAKE_AGENT_H_
