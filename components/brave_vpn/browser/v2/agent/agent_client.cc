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

// How long connecting may keep failing before a customer is told, for failures
// a retry could still fix.
constexpr base::TimeDelta kPersistentFailureThreshold = base::Seconds(15);

// How long a session has to last to count as evidence that connecting works.
constexpr base::TimeDelta kMinStableSession = base::Seconds(3);

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
    return base::unexpected(ConnectFailure::kNoAgentRunning);
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
      // was replaced mid-update, a signing cert rotated, the accept-time
      // capture expired. None is a verdict about this binary, and a fresh
      // connection may well succeed.
      TeardownAndRetry(Error::kBrowserUnverified);
      return;

    case mojom::BrowserAuthResult::kVersionMismatch:
      // The agent accepts a range ending at the version it was built with, so
      // this is usually a browser that updated ahead of the agent it is talking
      // to. Retrying against this agent cannot help; a replacement can.
      VLOG(1) << "Protocol version outside the agent's accepted range";
      EnterUnavailable(Error::kBrowserRejected);
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
  }
}

void AgentClient::OnHandshakeTimeout() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
  provider_.reset();
}

}  // namespace brave_vpn::v2
