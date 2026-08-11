/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/test/fake_agent.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "build/build_config.h"

namespace brave_vpn::v2 {

namespace {
#if BUILDFLAG(IS_WIN)
constexpr wchar_t kFakeServerName[] = L"fake-agent";
#else
constexpr char kFakeServerName[] = "fake-agent";
#endif
}  // namespace

FakeAgent::FakeAgent() = default;

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

void FakeAgent::set_auth_result(
    std::optional<mojom::BrowserAuthResult> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auth_result_ = result;
}

void FakeAgent::AnswerHeldRequest(mojom::BrowserAuthResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(held_reply_) << "No withheld request to answer";
  std::move(held_reply_).Run(result);
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
  held_reply_.Reset();
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

bool FakeAgent::has_held_request() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !held_reply_.is_null();
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

mojo::ScopedMessagePipeHandle FakeAgent::Connect(
    scoped_refptr<base::SequencedTaskRunner> agent_task_runner,
    const mojo::NamedPlatformChannel::ServerName& server_name) {
  // Deliberately no sequence check: this runs on the thread pool, like the
  // production connector, and touches only the atomics.
  connect_attempts_.fetch_add(1);
  if (transport_fails_.load()) {
    return mojo::ScopedMessagePipeHandle();
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
  return std::move(pipe.handle1);
}

void FakeAgent::BindProvider(mojo::ScopedMessagePipeHandle pipe) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  provider_receivers_.Add(
      this, mojo::PendingReceiver<mojom::BrowserHostProvider>(std::move(pipe)));
}

void FakeAgent::BindBrowserHost(
    uint32_t protocol_version,
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> host,
    BindBrowserHostCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++bind_browser_host_calls_;
  last_protocol_version_ = protocol_version;

  if (!auth_result_) {
    // An agent that has taken the connection and gone quiet. The handles are
    // kept so that a test can decide when, and whether, they go away.
    //
    // A withheld reply may already be here: the client times the handshake out
    // and retries, which arrives as a second request. The earlier one belongs
    // to a connection the client has since dropped, so answering it could reach
    // nobody; the newest request is the only one worth holding.
    held_reply_ = std::move(callback);
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
