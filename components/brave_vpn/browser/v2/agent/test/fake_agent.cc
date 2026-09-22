/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/test/fake_agent.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_vpn/browser/v2/agent/test/fake_agent_identity.h"
#include "build/build_config.h"

namespace brave_vpn::v2 {

namespace {
#if BUILDFLAG(IS_WIN)
constexpr wchar_t kFakeServerName[] = L"fake-agent";
#else
constexpr char kFakeServerName[] = "fake-agent";
#endif
}  // namespace

FakeAgent::FakeAgent() {
  // The real agent drops its per-connection state when the connection goes
  // away; mirroring that keeps |initialized_| consistent with
  // connection_count().
  provider_receivers_.set_disconnect_handler(base::BindRepeating(
      &FakeAgent::OnProviderDisconnected, base::Unretained(this)));
}

FakeAgent::~FakeAgent() = default;

AgentClient::ServerNameProvider FakeAgent::GetServerNameProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::BindRepeating(&FakeAgent::GetServerName, base::Unretained(this));
}

AgentClient::Connector FakeAgent::GetConnector() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::BindRepeating(&FakeAgent::Connect, base::Unretained(this),
                             base::SequencedTaskRunner::GetCurrentDefault());
}

void FakeAgent::set_init_result(
    std::optional<mojom::BrowserInitResult> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  init_result_ = result;
}

void FakeAgent::set_auth_result(
    std::optional<mojom::BrowserAuthResult> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auth_result_ = result;
}

void FakeAgent::AnswerHeldAuthRequest(mojom::BrowserAuthResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(held_auth_reply_) << "No withheld BindBrowserHost() to answer";
  std::move(held_auth_reply_).Run(result);
}

void FakeAgent::DropSessionHandles() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  browser_endpoints_.clear();
  parked_hosts_.clear();
}

void FakeAgent::CloseAllConnections() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DropSessionHandles();
  provider_receivers_.Clear();
  held_init_reply_.Reset();
  held_auth_reply_.Reset();
  initialized_.clear();
}

int FakeAgent::initialize_calls() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return initialize_calls_;
}

int FakeAgent::bind_browser_host_calls() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return bind_browser_host_calls_;
}

std::optional<uint32_t> FakeAgent::last_protocol_version() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return last_protocol_version_;
}

size_t FakeAgent::connection_count() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return provider_receivers_.size();
}

size_t FakeAgent::session_count() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return browser_endpoints_.size();
}

bool FakeAgent::has_held_init_request() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !held_init_reply_.is_null();
}

bool FakeAgent::has_held_auth_request() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !held_auth_reply_.is_null();
}

void FakeAgent::OnProviderDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  initialized_.erase(provider_receivers_.current_receiver());
}

std::optional<mojo::NamedPlatformChannel::ServerName>
FakeAgent::GetServerName() {
  // Deliberately no sequence check: runs on the thread pool. The name is never
  // looked up - Connect() ignores it - so its only job is to be present or
  // absent.
  if (!server_name_available_.load()) {
    return std::nullopt;
  }
  return mojo::NamedPlatformChannel::ServerName(kFakeServerName);
}

std::optional<AgentClient::Transport> FakeAgent::Connect(
    scoped_refptr<base::SequencedTaskRunner> agent_task_runner,
    const mojo::NamedPlatformChannel::ServerName& server_name) {
  // Deliberately no sequence check: this runs on the thread pool, like the
  // production connector. Touches only the atomics and the fake identity it
  // builds from them.
  connect_attempts_.fetch_add(1);
  if (transport_fails_.load()) {
    return std::nullopt;
  }

  AgentClient::Transport transport;
  if (const std::optional<AgentIdentity::VerificationResult> result =
          identity_result_.load()) {
    transport.identity = std::make_unique<FakeAgentIdentity>(*result);
  }

  // Stands in for a connected channel plus an accepted invitation. Created here
  // rather than on the agent's sequence so that the handle reaches the client
  // the same way the real one does: as the return value of an off-sequence
  // call.
  mojo::MessagePipe pipe;
  agent_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&FakeAgent::BindProvider, base::Unretained(this),
                     std::move(pipe.handle0)));
  transport.pipe = std::move(pipe.handle1);
  return transport;
}

void FakeAgent::BindProvider(mojo::ScopedMessagePipeHandle pipe) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  provider_receivers_.Add(
      this, mojo::PendingReceiver<mojom::BrowserHostProvider>(std::move(pipe)));
}

void FakeAgent::Initialize(uint32_t protocol_version,
                           mojo::PlatformHandle /*identity_channel*/,
                           InitializeCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++initialize_calls_;
  last_protocol_version_ = protocol_version;

  const mojo::ReceiverId receiver_id = provider_receivers_.current_receiver();
  if (initialized_.contains(receiver_id)) {
    // One successful Initialize() per connection, as in the real agent.
    std::move(callback).Run(mojom::BrowserInitResult::kInvalidRequest);
    return;
  }

  if (!init_result_) {
    // An agent that took the connection and went quiet before the handshake
    // got anywhere, which is what the client's handshake timeout covers.
    held_init_reply_ = std::move(callback);
    return;
  }

  if (*init_result_ == mojom::BrowserInitResult::kInitialized) {
    initialized_.insert(receiver_id);
  }
  std::move(callback).Run(*init_result_);
}

void FakeAgent::BindBrowserHost(
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host,
    BindBrowserHostCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++bind_browser_host_calls_;

  if (!initialized_.contains(provider_receivers_.current_receiver())) {
    // The real agent has no per-connection entry to bind against until
    // Initialize() succeeds. Both handles go out of scope here, as they do
    // there.
    std::move(callback).Run(mojom::BrowserAuthResult::kInvalidRequest);
    return;
  }

  if (!auth_result_) {
    // An agent that has taken the connection and gone quiet. The handles are
    // kept so that a test can decide when, and whether, they go away.
    //
    // A withheld reply may already be here: the client times the handshake out
    // and retries, which arrives as a second request. The earlier one belongs
    // to a connection the client has since dropped, so answering it could reach
    // nobody; the newest request is the only one worth holding.
    held_auth_reply_ = std::move(callback);
    KeepSession(std::move(browser_endpoint), std::move(host));
    return;
  }

  if (*auth_result_ == mojom::BrowserAuthResult::kAccepted) {
    KeepSession(std::move(browser_endpoint), std::move(host));
  }
  // On any other reply both handles go out of scope here, as they do in the
  // real agent, which is what makes a refusal race its own reply.
  std::move(callback).Run(*auth_result_);
}

void FakeAgent::KeepSession(
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  browser_endpoints_.emplace_back(std::move(browser_endpoint));
  parked_hosts_.push_back(std::move(host));
}

}  // namespace brave_vpn::v2
