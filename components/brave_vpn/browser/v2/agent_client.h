/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_CLIENT_H_

#include <memory>
#include <optional>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "base/time/tick_clock.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "net/base/backoff_entry.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS));

namespace brave_vpn::v2 {

class BrowserEndpointImpl;

// The browser end of the browser to agent contract: owns the single connection
// a VPN keyed service has to the user's agent, drives the handshake, and hands
// out the authenticated BrowserHost once the agent accepts.
//
// Owned per eligible profile, by a VPN keyed service, and constructed on that
// service's sequence; every method must then be called on that sequence.
// Several instances per browser process is the expected arrangement, not a
// mistake to guard against: the agent keys sessions by connection, so each
// profile gets its own connection/handshake/API surface, and the agent is what
// reconciles them.
//
// There is one agent per user and many clients per agent - every eligible
// profile of every browser instance the user has open - so from here the agent
// looks like a shared service that may not be running yet and may go away at
// any time. Connecting is therefore lazy and failure is routine: a failed
// connect or a dropped pipe is retried with exponential backoff and jitter. The
// jitter is load-bearing, because the events worth retrying after are exactly
// the ones every client sees at the same instant: the agent restarting, or a
// browser launching several profiles at once.
class AgentClient {
 public:
  enum class State {
    // Not connected means nothing bound and no attempt scheduled; this is the
    // initial state, and also the state after a reset.
    kDisconnected,
    // Transport connect or handshake in flight.
    kConnecting,
    // The agent accepted the connection; browser host is valid.
    kConnected,
    // Transient failure; a retry is on the timer.
    kWaitingToRetry,
    // No attempt is scheduled, because the last one failed in a way that
    // retrying cannot change: either the agent refused the browser, or there
    // is no server name to connect to at all. In the first case the provider
    // pipe is still held and this state is left on its own when that agent
    // process exits; in the second nothing is held and only a reset leaves.
    kUnavailable,
  };

  class Observer : public base::CheckedObserver {
   public:
    // The handshake succeeded; browser host is now usable.
    virtual void OnAgentConnected() {}
    // A previously connected session went away. Any browser host pointer handed
    // out earlier is dangling from here on.
    virtual void OnAgentDisconnected() {}
    // Entered "unavailable" state. |result| is the agent's reply, or nullopt
    // when the failure was not an auth result - today, no server name for this
    // session. That distinction is also the recovery rule: with a reply there
    // was a live agent; with nullopt nothing will change without a reset.
    virtual void OnAgentUnavailable(
        std::optional<mojom::BrowserAuthResult> result) {}
    // The agent is not reachable at all, and the client will retry on its own,
    // while the observing code can start (or restart) the agent.
    virtual void OnAgentNotRunning() {}
  };

  // Resolves the agent's server name; runs on a blocking sequence.
  using ServerNameProvider = base::RepeatingCallback<
      std::optional<mojo::NamedPlatformChannel::ServerName>()>;

  // Establishes the transport to the agent listening on |server_name| and
  // returns the pipe its BrowserHostProvider is bound to, or an invalid handle
  // on failure; runs on a blocking sequence.
  using Connector = base::RepeatingCallback<mojo::ScopedMessagePipeHandle(
      const mojo::NamedPlatformChannel::ServerName&)>;

  AgentClient();
  ~AgentClient();

  AgentClient(const AgentClient&) = delete;
  AgentClient& operator=(const AgentClient&) = delete;

  // The test seam lets a fake agent be reached without a real socket or pipe.
  // |tick_clock| overrides the backoff's clock; tests using MOCK_TIME must pass
  // TaskEnvironment's mock tick clock, or the retry paths will not advance.
  static std::unique_ptr<AgentClient> CreateForTesting(
      ServerNameProvider server_name_provider,
      Connector connector,
      const base::TickClock* tick_clock);

  // Starts connecting if nothing is in flight, connected, or already scheduled.
  // Idempotent and safe to call from every entry point that needs the agent. In
  // the "unavailable" state it does nothing: that state either recovers by
  // itself or needs a reset, and calling again cannot help either way.
  void EnsureConnected();

  // Drops the connection, cancels any scheduled retry, clears the backoff and
  // any terminal state, and returns to the "disconnected" state. Notifies
  // observers if a session was live.
  void Reset();

  // The authenticated surface, or nullptr unless state() is "connected". Use it
  // and drop it within the same task: it is invalidated by disconnection, which
  // observers learn about through OnAgentDisconnected().
  mojom::BrowserHost* browser_host();

  State state() const;
  bool is_connected() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  AgentClient(ServerNameProvider server_name_provider,
              Connector connector,
              const base::TickClock* tick_clock = nullptr);

  enum class ConnectError {
    // No valid server name could be obtained (no GUI session on Linux, or a
    // path too long for sockaddr_un); cannot be fixed by the service, so this
    // is a fatal non-retryable error.
    kNoServerName,
    // There is a valid server name, but nothing listening on it: the agent is
    // likely not running, and the service should start it;  this is a transient
    // error, so retrying is appropriate.
    kNoAgentRunning,
  };
  using ConnectResult =
      base::expected<mojo::ScopedMessagePipeHandle, ConnectError>;

  // Resolves the server name and connects, both blocking. Static because it
  // runs off-sequence, where |this| must not be touched.
  static ConnectResult ConnectBlocking(ServerNameProvider server_name_provider,
                                       Connector connector);

  void StartConnect();
  void OnConnectBlockingCompleted(ConnectResult result);
  void OnAuthResult(mojom::BrowserAuthResult result);
  void OnHandshakeTimeout();
  void OnSessionPipeDisconnected(std::string_view reason);
  void OnProviderDisconnected();
  void TeardownAndRetry(std::string_view reason);
  void EnterUnavailable(std::string_view reason,
                        std::optional<mojom::BrowserAuthResult> result);
  void ReportNotRunningIfNeeded();
  void ClearFailureRun();
  void ResetSession();
  void ResetConnection();

  SEQUENCE_CHECKER(sequence_checker_);
  const ServerNameProvider server_name_provider_;
  const Connector connector_;
  State state_ = State::kDisconnected;
  net::BackoffEntry backoff_;
  bool not_running_reported_ = false;
  bool session_pipe_dropped_ = false;
  mojo::Remote<mojom::BrowserHostProvider> provider_;
  mojo::Remote<mojom::BrowserHost> host_;
  std::unique_ptr<BrowserEndpointImpl> browser_endpoint_;
  base::OneShotTimer handshake_timer_;
  base::OneShotTimer retry_timer_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<AgentClient> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_AGENT_CLIENT_H_
