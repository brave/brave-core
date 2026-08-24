/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent_client.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/browser_endpoint_impl.h"
#include "brave/components/brave_vpn/common/v2/agent_utils.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server_client_util.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace brave_vpn::v2 {
namespace {
// Back off policy for the connection to an agent. The common first-time failure
// is "no agent running", which an immediate retry will not fix. Jitter is
// load-bearing: one agent serves every profile of every browser instance the
// user has open, and they fail together, so an agent restart would otherwise be
// answered by all of them reconnecting in lockstep, repeatedly, on a schedule
// they all derived the same way.
constexpr net::BackoffEntry::Policy kBackoffPolicy = {
    .num_errors_to_ignore = 0,
    .initial_delay_ms = 1000,
    .multiply_factor = 1.5,
    .jitter_factor = 0.5,
    .maximum_backoff_ms = 60 * 1000,
    .entry_lifetime_ms = -1,
    .always_use_initial_delay = false,
};

// How long the agent gets to answer BindBrowserHost() before the connection is
// treated as failed.
constexpr base::TimeDelta kHandshakeTimeout = base::Seconds(10);

// A helper that establishes the transport to the agent's IPC server and returns
// the pipe its BrowserHostProvider is bound to.
mojo::ScopedMessagePipeHandle ConnectToAgentServer(
    const mojo::NamedPlatformChannel::ServerName& server_name) {
  mojo::PlatformChannelEndpoint endpoint =
      named_mojo_ipc_server::ConnectToServer(server_name);
  if (!endpoint.is_valid()) {
    // The ordinary case on a cold browser: nothing is listening yet.
    return mojo::ScopedMessagePipeHandle();
  }
  mojo::ScopedMessagePipeHandle pipe =
      mojo::IncomingInvitation::AcceptIsolated(std::move(endpoint));
  if (!pipe.is_valid()) {
    // Reached the agent's channel but couldn't turn it into a connection, which
    // is not something a retry usually fixes. Logged here because this is the
    // only place that can tell the two failures apart.
    LOG(ERROR) << "Agent did not accept an isolated invitation";
  }
  return pipe;
}

}  // namespace

AgentClient::AgentClient()
    : AgentClient(base::BindRepeating(&GetAgentServerName),
                  base::BindRepeating(&ConnectToAgentServer)) {}

AgentClient::AgentClient(ServerNameProvider server_name_provider,
                         Connector connector,
                         const base::TickClock* tick_clock)
    : server_name_provider_(std::move(server_name_provider)),
      connector_(std::move(connector)),
      backoff_(&kBackoffPolicy, tick_clock) {
  CHECK(server_name_provider_);
  CHECK(connector_);
}

AgentClient::~AgentClient() = default;

// static
std::unique_ptr<AgentClient> AgentClient::CreateForTesting(  // IN-TEST
    ServerNameProvider server_name_provider,
    Connector connector,
    const base::TickClock* tick_clock) {
  return base::WrapUnique(new AgentClient(std::move(server_name_provider),
                                          std::move(connector), tick_clock));
}

void AgentClient::EnsureConnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (state_) {
    case State::kConnecting:
    case State::kConnected:
    case State::kWaitingToRetry:
      break;
    case State::kUnavailable:
      VLOG(1) << "Agent is unavailable to this browser; not reconnecting";
      break;
    case State::kDisconnected:
      StartConnect();
      break;
  }
}

void AgentClient::Reset() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const bool was_connected = state_ == State::kConnected;
  retry_timer_.Stop();
  weak_factory_.InvalidateWeakPtrs();
  ResetConnection();
  ClearFailureRun();
  state_ = State::kDisconnected;
  if (was_connected) {
    observers_.Notify(&Observer::OnAgentDisconnected);
  }
}

mojom::BrowserHost* AgentClient::browser_host() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // The host pipe is bound before the handshake completes, so gate on the state
  // rather than on the remote: sending to an unauthenticated host would just be
  // queued into a pipe the agent is about to close.
  return state_ == State::kConnected ? host_.get() : nullptr;
}

AgentClient::State AgentClient::state() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_;
}

bool AgentClient::is_connected() const {
  return state() == State::kConnected;
}

void AgentClient::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void AgentClient::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

// static
AgentClient::ConnectResult AgentClient::ConnectBlocking(
    ServerNameProvider server_name_provider,
    Connector connector) {
  std::optional<mojo::NamedPlatformChannel::ServerName> server_name =
      server_name_provider.Run();
  if (!server_name) {
    // Already logged by the resolver.
    return base::unexpected(ConnectError::kNoServerName);
  }

  // TODO(https://github.com/brave/brave-browser/issues/54608)
  // Verify the agent server's identity here, before the endpoint is
  // handed to mojo. The agent verifies the browser after BindBrowserHost()
  // arrives, but this direction is unchecked: a malicious process in the
  // session can create a pipe with the agent's name, and the name is
  // enumerable. The handle is still a plain pipe at this point, so the server
  // pid can be fetched and fed to the same signature check the agent uses to
  // verify the browser. Until then, treat everything reachable through this
  // connection as talking to a peer we have not authenticated.

  mojo::ScopedMessagePipeHandle pipe = connector.Run(*server_name);
  if (!pipe.is_valid()) {
    return base::unexpected(ConnectError::kNoAgentRunning);
  }
  return pipe;
}

void AgentClient::StartConnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!provider_.is_bound());
  state_ = State::kConnecting;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&AgentClient::ConnectBlocking, server_name_provider_,
                     connector_),
      base::BindOnce(&AgentClient::OnConnectBlockingCompleted,
                     weak_factory_.GetWeakPtr()));
}

void AgentClient::OnConnectBlockingCompleted(ConnectResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(state_ == State::kConnecting);

  if (!result.has_value()) {
    if (result.error() == ConnectError::kNoServerName) {
      // Terminal rather than retried: the session id, runtime dir, and temp dir
      // this is derived from are fixed for the life of the process, so every
      // later attempt would fail identically.
      EnterUnavailable("no agent server name for this session", std::nullopt);
      return;
    }
    TeardownAndRetry("couldn't reach the agent");
    ReportNotRunningIfNeeded();
    return;
  }

  // The pipe the transport was established on is what the agent bound its
  // BrowserHostProvider to. Dropping it later drops the transport with it. Pass
  // 0 as version as we don't support [MinVersion]-based versioning yet.
  provider_.Bind(mojo::PendingRemote<mojom::BrowserHostProvider>(
      std::move(result.value()), /*version=*/0));

  provider_.set_disconnect_handler(base::BindOnce(
      &AgentClient::OnProviderDisconnected, weak_factory_.GetWeakPtr()));

  browser_endpoint_ = std::make_unique<BrowserEndpointImpl>(
      base::BindOnce(&AgentClient::OnSessionPipeDisconnected,
                     weak_factory_.GetWeakPtr(), "endpoint pipe closed"));
  mojo::PendingRemote<mojom::BrowserEndpoint> endpoint_remote =
      browser_endpoint_->BindNewPipeAndPassRemote();

  mojo::PendingReceiver<mojom::BrowserHost> host_receiver =
      host_.BindNewPipeAndPassReceiver();
  host_.set_disconnect_handler(
      base::BindOnce(&AgentClient::OnSessionPipeDisconnected,
                     weak_factory_.GetWeakPtr(), "host pipe closed"));

  handshake_timer_.Start(FROM_HERE, kHandshakeTimeout,
                         base::BindOnce(&AgentClient::OnHandshakeTimeout,
                                        weak_factory_.GetWeakPtr()));

  provider_->BindBrowserHost(
      mojom::kProtocolVersion, std::move(endpoint_remote),
      std::move(host_receiver),
      base::BindOnce(&AgentClient::OnAuthResult, weak_factory_.GetWeakPtr()));
}

void AgentClient::OnAuthResult(mojom::BrowserAuthResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(state_ == State::kConnecting);
  handshake_timer_.Stop();

  // This value came off the wire from a peer that has not been verified, so it
  // decides policy but never invariants: no CHECK on any branch below.
  switch (result) {
    case mojom::BrowserAuthResult::kAccepted:
      if (session_pipe_dropped_) {
        // Accepted, but one of the pipes the session runs on is already gone.
        // Treat it as a failed attempt rather than publishing a host that
        // cannot deliver anything.
        TeardownAndRetry("session pipe closed during handshake");
        return;
      }
      ClearFailureRun();
      state_ = State::kConnected;
      observers_.Notify(&Observer::OnAgentConnected);
      return;

    case mojom::BrowserAuthResult::kVersionMismatch:
      // The agent accepts a range ending at the version it was built with, so
      // this is usually a browser that updated ahead of the agent it is talking
      // to. Retrying against this agent cannot help; a replacement can.
      EnterUnavailable("protocol version outside the agent's accepted range",
                       result);
      return;

    case mojom::BrowserAuthResult::kRejected:
      // The agent ran its peer check on us and said no. That verdict is about
      // this binary, which does not change while it runs, so back off entirely.
      EnterUnavailable("agent rejected this browser", result);
      return;

    case mojom::BrowserAuthResult::kHostAlreadyRequested:
      // The agent scopes this to one BindBrowserHost() per connection, and this
      // is a connection we have just opened and called once, so either this is
      // a bug or the peer is not the agent.
      LOG(ERROR) << "Agent reports this connection is already authenticated";
      EnterUnavailable("connection already authenticated", result);
      return;
  }
}

void AgentClient::OnHandshakeTimeout() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TeardownAndRetry("agent did not answer the handshake");
}

void AgentClient::OnProviderDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ == State::kUnavailable) {
    // The agent that refused us is gone. Its replacement is a different binary
    // with a potentially different answer, so this is not a failure to back off
    // from. Connect as if for the first time. This is how a browser that
    // outlives an agent update recovers without being restarted.
    ClearFailureRun();
    ResetConnection();
    state_ = State::kDisconnected;
    StartConnect();
    return;
  }
  TeardownAndRetry("provider pipe closed");
}

void AgentClient::OnSessionPipeDisconnected(std::string_view reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ == State::kConnecting) {
    // The reply decides the outcome, and the provider pipe's own handler covers
    // the case where no reply is coming.
    VLOG(1) << "Session pipe dropped during the handshake (" << reason << ")";
    session_pipe_dropped_ = true;
    return;
  }
  TeardownAndRetry(reason);
}

void AgentClient::TeardownAndRetry(std::string_view reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const bool was_connected = state_ == State::kConnected;
  ResetConnection();
  if (was_connected) {
    ClearFailureRun();
  }
  backoff_.InformOfRequest(/*succeeded=*/false);

  const base::TimeDelta delay = backoff_.GetTimeUntilRelease();
  VLOG(1) << "Agent connection failed (" << reason << "); retrying in "
          << delay;

  state_ = State::kWaitingToRetry;
  retry_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&AgentClient::StartConnect, weak_factory_.GetWeakPtr()));

  if (was_connected) {
    observers_.Notify(&Observer::OnAgentDisconnected);
  }
}

void AgentClient::EnterUnavailable(
    std::string_view reason,
    std::optional<mojom::BrowserAuthResult> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const bool was_connected = state_ == State::kConnected;

  retry_timer_.Stop();
  ResetSession();
  state_ = State::kUnavailable;
  VLOG(1) << "Agent unavailable to this browser: " << reason;

  if (was_connected) {
    observers_.Notify(&Observer::OnAgentDisconnected);
  }
  observers_.Notify(&Observer::OnAgentUnavailable, result);
}

void AgentClient::ReportNotRunningIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (std::exchange(not_running_reported_, true)) {
    return;
  }
  observers_.Notify(&Observer::OnAgentNotRunning);
}

void AgentClient::ClearFailureRun() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  backoff_.Reset();
  not_running_reported_ = false;
}

void AgentClient::ResetSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  handshake_timer_.Stop();
  session_pipe_dropped_ = false;
  host_.reset();
  browser_endpoint_.reset();
}

void AgentClient::ResetConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ResetSession();
  provider_.reset();
}

}  // namespace brave_vpn::v2
