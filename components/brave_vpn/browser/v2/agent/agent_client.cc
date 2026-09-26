/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_client.h"

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
#include "brave/components/brave_vpn/browser/v2/agent/browser_endpoint_impl.h"
#include "brave/components/brave_vpn/common/v2/agent_utils.h"
#include "components/named_mojo_ipc_server/named_mojo_ipc_server_client_util.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/platform/platform_handle.h"
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

// How long the agent gets to finish the handshake: Initialize(), verifying the
// agent, then BindBrowserHost(), before the connection is treated as failed.
constexpr base::TimeDelta kHandshakeTimeout = base::Seconds(15);

// How long connecting may keep failing before a customer is told, for failures
// a retry could still fix.
constexpr base::TimeDelta kPersistentFailureThreshold = base::Seconds(15);

// How long a session has to last to count as evidence that connecting works.
constexpr base::TimeDelta kMinStableSession = base::Seconds(3);

// A helper that establishes the transport to the agent's IPC server and returns
// the pipe its BrowserHostProvider is bound to.
std::optional<AgentClient::Transport> ConnectToAgentServer(
    const mojo::NamedPlatformChannel::ServerName& server_name) {
  mojo::PlatformChannelEndpoint endpoint =
      named_mojo_ipc_server::ConnectToServer(server_name);
  if (!endpoint.is_valid()) {
    // The ordinary case on a cold browser: nothing is listening yet.
    return std::nullopt;
  }
  // Identity capture should happen before the invitation is accepted, because
  // that consumes the endpoint and with it any chance of pinning the peer from
  // the transport.
  AgentClient::Transport transport;
  transport.identity = AgentIdentity::Create(endpoint);
  transport.pipe =
      mojo::IncomingInvitation::AcceptIsolated(std::move(endpoint));
  if (!transport.pipe.is_valid()) {
    // Reached the agent's channel but couldn't turn it into a connection, which
    // is not something a retry usually fixes. Logged here because this is the
    // only place that can tell the two failures apart.
    LOG(ERROR) << "Agent did not accept an isolated invitation";
    return std::nullopt;
  }
  return transport;
}

bool IsRetryableError(AgentClient::Error error) {
  switch (error) {
    case AgentClient::Error::kAgentUnreachable:
    case AgentClient::Error::kAgentNotResponding:
    case AgentClient::Error::kAgentUnstable:
    case AgentClient::Error::kBrowserUnverified:
      // All four describe a situation, not a verdict: the agent may yet start,
      // stop wedging, stay up, or manage to verify this browser.
      return true;
    case AgentClient::Error::kNoEndpoint:
    case AgentClient::Error::kBrowserRejected:
    case AgentClient::Error::kUnexpectedBehavior:
      // Settled for as long as this browser and this agent are both running.
      // Recovery needs the agent replaced, or a reset from the owner.
      return false;
  }
}

}  // namespace

// static
std::string_view AgentClient::ErrorToString(Error error) {
  switch (error) {
    case AgentClient::Error::kNoEndpoint:
      return "no endpoint for this session";
    case AgentClient::Error::kAgentUnreachable:
      return "agent unreachable";
    case AgentClient::Error::kAgentNotResponding:
      return "agent not responding";
    case AgentClient::Error::kAgentUnstable:
      return "agent unstable";
    case AgentClient::Error::kBrowserUnverified:
      return "browser could not be verified";
    case AgentClient::Error::kBrowserRejected:
      return "browser rejected by the agent";
    case AgentClient::Error::kUnexpectedBehavior:
      return "unexpected peer behavior";
  }
}

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
    return base::unexpected(ConnectFailure::kNoServerName);
  }
  std::optional<Transport> transport = connector.Run(*server_name);
  if (!transport || !transport->pipe.is_valid()) {
    return base::unexpected(ConnectFailure::kNoAgentRunning);
  }
  return std::move(*transport);
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
    if (result.error() == ConnectFailure::kNoServerName) {
      // Terminal rather than retried: the session id, runtime dir, and temp dir
      // this is derived from are fixed for the life of the process, so every
      // later attempt would fail identically.
      EnterUnavailable(Error::kNoEndpoint);
      return;
    }
    TeardownAndRetry(Error::kAgentUnreachable);
    ReportNotRunningIfNeeded();
    return;
  }

  // Reaching the agent indicates that it is running, but only a session that
  // lasts is evidence that connecting works.
  not_running_reported_ = false;

  // The transport is up but the peer could not be pinned, so there is nothing
  // to verify the agent against. Local and likely transient, hence retryable.
  agent_identity_ = std::move(result->identity);
  if (!agent_identity_) {
    VLOG(1) << "Could not capture the agent's identity";
    TeardownAndRetry(Error::kAgentNotResponding);
    return;
  }

  // The pipe the transport was established on is what the agent bound its
  // BrowserHostProvider to. Dropping it later drops the transport with it. Pass
  // 0 as version as we don't support [MinVersion]-based versioning yet.
  provider_.Bind(mojo::PendingRemote<mojom::BrowserHostProvider>(
      std::move(result->pipe), /*version=*/0));

  provider_.set_disconnect_handler(base::BindOnce(
      &AgentClient::OnProviderDisconnected, weak_factory_.GetWeakPtr()));

  handshake_timer_.Start(FROM_HERE, kHandshakeTimeout,
                         base::BindOnce(&AgentClient::OnHandshakeTimeout,
                                        weak_factory_.GetWeakPtr()));

  provider_->Initialize(
      mojom::kProtocolVersion, agent_identity_->TakeSendHandle(),
      base::BindOnce(&AgentClient::OnInitResult, weak_factory_.GetWeakPtr()));
}

void AgentClient::OnInitResult(mojom::BrowserInitResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(state_ == State::kConnecting);
  // The handshake timer deliberately keeps running: one budget covers both
  // Initialize() and BindBrowserHost(), and a handshake assumes that both
  // happen sequentially. The refusal paths stop the timer through a session
  // reset call.

  // |result| comes off the wire from an unverified peer, so the branches below
  // decide policy and never assert: no CHECKs on the value.
  switch (result) {
    case mojom::BrowserInitResult::kInitialized:
      break;

    case mojom::BrowserInitResult::kVersionMismatch:
      // The agent accepts a range ending at the version it was built with, so
      // this is usually a browser that updated ahead of the agent it is talking
      // to. Retrying against this agent cannot help; a replacement can.
      VLOG(1) << "Protocol version outside the agent's accepted range";
      EnterUnavailable(Error::kBrowserRejected);
      return;

    case mojom::BrowserInitResult::kNotIdentified:
      // The agent has no usable capture of this process: the accept-time one
      // expired, or its pid resolved to another process. Not a verdict about
      // this binary, and only a new connection can produce a fresh capture, so
      // retry rather than re-asking on this one.
      VLOG(1) << "Agent could not identify the connection";
      TeardownAndRetry(Error::kBrowserUnverified);
      return;

    case mojom::BrowserInitResult::kInvalidRequest:
      // The agent scopes this to one Initialize() per connection, and this is a
      // connection we have just opened and called once, so either this is a bug
      // or the peer is not the agent.
      VLOG(1) << "Agent reports an invalid initialization request";
      EnterUnavailable(Error::kUnexpectedBehavior);
      return;
  }

  // Verify the agent before handing it anything.
  agent_identity_->Verify(base::BindOnce(
      &AgentClient::OnAgentVerified, connection_weak_factory_.GetWeakPtr()));
}

void AgentClient::OnAgentVerified(AgentIdentity::VerificationResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Unlike the Mojo handshake replies, this one is not carried by |provider_|,
  // and it is cancelled by the weak pointer invalidation rather than by
  // dropping the remote. The check covers arriving in a state no longer
  // expecting it, which is a condition rather than an invariant.
  if (state_ != State::kConnecting) {
    return;
  }

  switch (result) {
    case AgentIdentity::VerificationResult::kAccepted:
      break;

    case AgentIdentity::VerificationResult::kRejected:
      // A verdict about the binary serving this connection, which does not
      // change while it runs.
      VLOG(1) << "Agent failed verification";
      EnterUnavailable(Error::kUnexpectedBehavior);
      return;

    case AgentIdentity::VerificationResult::kInconclusive:
      // No identity message, image replaced mid-update, cert rotated. Not a
      // verdict, so a fresh connection may succeed.
      VLOG(1) << "Agent could not be verified";
      TeardownAndRetry(Error::kAgentNotResponding);
      return;
  }

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
  provider_->BindBrowserHost(
      std::move(endpoint_remote), std::move(host_receiver),
      base::BindOnce(&AgentClient::OnAuthResult, weak_factory_.GetWeakPtr()));
}

void AgentClient::OnAuthResult(mojom::BrowserAuthResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(state_ == State::kConnecting);
  handshake_timer_.Stop();

  // This value came off the wire from a peer that has not been verified, so it
  // decides policy but never invariants: no CHECK on the branches.
  switch (result) {
    case mojom::BrowserAuthResult::kAccepted:
      if (session_pipe_dropped_) {
        // Accepted, but one of the pipes the session runs on is already gone.
        // Treat it as a failed attempt rather than publishing a host that
        // cannot deliver anything.
        VLOG(1) << "Agent session pipe closed during handshake";
        TeardownAndRetry(Error::kAgentUnstable);
        return;
      }
      state_ = State::kConnected;
      stable_session_timer_.Start(
          FROM_HERE, kMinStableSession,
          base::BindOnce(&AgentClient::OnSessionBecameStable,
                         weak_factory_.GetWeakPtr()));
      observers_.Notify(&Observer::OnAgentConnected);
      return;

    case mojom::BrowserAuthResult::kInconclusive:
      // The agent couldn't determine whether this browser is Brave: its image
      // was replaced mid-update, a signing cert rotated. Neither is a verdict
      // about this binary, and a fresh connection may well succeed.
      TeardownAndRetry(Error::kBrowserUnverified);
      return;

    case mojom::BrowserAuthResult::kRejected:
      // The agent ran its peer check on us and said no. That verdict is about
      // this binary, which does not change while it runs, so back off entirely.
      VLOG(1) << "Agent rejected the browser";
      EnterUnavailable(Error::kBrowserRejected);
      return;

    case mojom::BrowserAuthResult::kHostAlreadyRequested:
      // The agent scopes this to one BindBrowserHost() per connection, and this
      // is a connection we have just opened and called once, so either this is
      // a bug or the peer is not the agent.
      VLOG(1) << "Agent reports the connection is already authenticated";
      EnterUnavailable(Error::kUnexpectedBehavior);
      return;

    case mojom::BrowserAuthResult::kInvalidRequest:
      // The agent received a request it could not understand. This is likely
      // a bug in the browser or a misbehaving peer.
      VLOG(1) << "Agent reports an invalid request";
      EnterUnavailable(Error::kUnexpectedBehavior);
      return;
  }
}

void AgentClient::OnHandshakeTimeout() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Every path that leaves kConnecting stops this timer through ResetSession(),
  // so a firing timer means the handshake is still in flight.
  CHECK(state_ == State::kConnecting);

  VLOG(1) << "Agent did not answer the handshake";
  TeardownAndRetry(Error::kAgentNotResponding);
}

void AgentClient::OnSessionBecameStable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ClearFailureRun();
  observers_.Notify(&Observer::OnAgentSessionStable);
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
  VLOG(1) << "Provider pipe dropped";
  TeardownAndRetry(Error::kAgentNotResponding);
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
  VLOG(1) << "Session pipe dropped: " << reason;
  TeardownAndRetry(Error::kAgentNotResponding);
}

void AgentClient::TeardownAndRetry(Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(IsRetryableError(error));
  const bool was_connected = state_ == State::kConnected;
  // A session still inside its stability window is not evidence that
  // reconnecting will work; it is the signature of an agent that cannot stay
  // up. A session that outlived the window already cleared the run itself.
  const bool was_unstable = was_connected && stable_session_timer_.IsRunning();
  if (error == Error::kAgentNotResponding && was_unstable) {
    error = Error::kAgentUnstable;
  }

  ResetConnection();
  if (!failure_run_timer_) {
    failure_run_timer_.emplace();
  }
  backoff_.InformOfRequest(/*succeeded=*/false);

  const base::TimeDelta delay = backoff_.GetTimeUntilRelease();
  VLOG(1) << "Agent connection failed (" << ErrorToString(error)
          << "); retrying in " << delay;

  state_ = State::kWaitingToRetry;
  retry_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&AgentClient::StartConnect, weak_factory_.GetWeakPtr()));

  if (was_connected) {
    observers_.Notify(&Observer::OnAgentDisconnected);
  }
  ReportErrorIfPersistent(error);
}

void AgentClient::EnterUnavailable(Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!IsRetryableError(error));
  const bool was_connected = state_ == State::kConnected;

  retry_timer_.Stop();
  ResetSession();
  state_ = State::kUnavailable;
  VLOG(1) << "Agent unavailable to this browser: " << ErrorToString(error);

  if (was_connected) {
    observers_.Notify(&Observer::OnAgentDisconnected);
  }
  ReportError(error);
}

void AgentClient::ReportNotRunningIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (std::exchange(not_running_reported_, true)) {
    return;
  }
  observers_.Notify(&Observer::OnAgentNotRunning);
}

void AgentClient::ReportError(Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // A run that keeps failing for shifting reasons is still one piece of news;
  // only an escalation to something retrying cannot fix is worth repeating.
  if (reported_error_ &&
      (*reported_error_ == error || IsRetryableError(error))) {
    return;
  }
  reported_error_ = error;
  observers_.Notify(&Observer::OnAgentConnectionFailed, error);
}

void AgentClient::ReportErrorIfPersistent(Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!failure_run_timer_ ||
      failure_run_timer_->Elapsed() < kPersistentFailureThreshold) {
    return;
  }
  ReportError(error);
}

void AgentClient::ClearFailureRun() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  backoff_.Reset();
  not_running_reported_ = false;
  failure_run_timer_.reset();
  reported_error_.reset();
}

void AgentClient::ResetSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  handshake_timer_.Stop();
  session_pipe_dropped_ = false;
  stable_session_timer_.Stop();
  host_.reset();
  browser_endpoint_.reset();
}

void AgentClient::ResetConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ResetSession();

  // Cancels everything still pointing at this attempt, including whatever comes
  // back from a thread pool. Every callback is rebound after this runs, so
  // nothing bound earlier may be needed later.
  connection_weak_factory_.InvalidateWeakPtrs();

  agent_identity_.reset();
  provider_.reset();
}

}  // namespace brave_vpn::v2
