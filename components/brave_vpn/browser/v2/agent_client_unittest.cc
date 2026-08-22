/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent_client.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/callback.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/test/fake_agent.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
// Time span long enough to run out any retry the client can schedule, since the
// backoff is capped: used where a test asserts that nothing further happens.
constexpr base::TimeDelta kTimePastEveryRetry = base::Minutes(60);

// Time span enough for a queued task or two to run without reaching any timer.
constexpr base::TimeDelta kTimeBriefly = base::Milliseconds(10);

// Time span between the first and second retry windows, to assert that the
// backoff is growing.
constexpr base::TimeDelta kTimeBetweenRetryWindows = base::Seconds(10);

class TestObserver : public AgentClient::Observer {
 public:
  void OnAgentConnected() override {
    ++connected_count_;
    Notify();
  }

  void OnAgentDisconnected() override {
    ++disconnected_count_;
    Notify();
  }

  void OnAgentUnavailable(
      std::optional<mojom::BrowserAuthResult> result) override {
    ++unavailable_count_;
    last_unavailable_result_ = result;
    Notify();
  }

  void OnAgentNotRunning() override {
    ++not_running_count_;
    Notify();
  }

  // Set to a RunLoop's quit closure for the duration of a wait, so that any
  // notification ends it: a test that takes the wrong path then fails on an
  // assertion instead of running out the harness timeout.
  void set_on_notification(base::RepeatingClosure on_notification) {
    on_notification_ = std::move(on_notification);
  }

  int connected_count() const { return connected_count_; }
  int disconnected_count() const { return disconnected_count_; }
  int unavailable_count() const { return unavailable_count_; }
  int not_running_count() const { return not_running_count_; }
  std::optional<mojom::BrowserAuthResult> last_unavailable_result() const {
    return last_unavailable_result_;
  }

 private:
  void Notify() {
    if (on_notification_) {
      on_notification_.Run();
    }
  }

  int connected_count_ = 0;
  int disconnected_count_ = 0;
  int unavailable_count_ = 0;
  int not_running_count_ = 0;
  std::optional<mojom::BrowserAuthResult> last_unavailable_result_;
  base::RepeatingClosure on_notification_;
};
}  // namespace

class AgentClientTest : public testing::Test {
 public:
  void SetUp() override { client_ = CreateClient(&observer_); }

  void TearDown() override {
    if (client_) {
      client_->RemoveObserver(&observer_);
      client_.reset();
    }
    // The connector runs on the thread pool holding an unretained pointer to
    // |agent_|, and posts back here to bind the provider. Both have to have run
    // before either member is destroyed: flush the pool, then this sequence.
    // Safe to drain fully, because the client is already gone and nothing is
    // left to schedule more work.
    base::ThreadPoolInstance::Get()->FlushForTesting();
    task_environment_.FastForwardUntilNoTasksRemain();
  }

 protected:
  std::unique_ptr<AgentClient> CreateClient(TestObserver* observer) {
    auto client = AgentClient::CreateForTesting(
        agent_.GetServerNameProvider(), agent_.GetConnector(),
        task_environment_.GetMockTickClock());
    client->AddObserver(observer);
    return client;
  }

  // Runs until the next notification of any kind reaches |observer_|.
  void WaitForNotification() {
    observer_.set_on_notification(task_environment_.QuitClosure());
    task_environment_.RunUntilQuit();
    observer_.set_on_notification(base::RepeatingClosure());
  }

  void ConnectAndWait() {
    client_->EnsureConnected();
    WaitForNotification();
  }

  // Every refusal is terminal in the same way, whatever the agent's reason.
  void ExpectRefusalIsTerminal(mojom::BrowserAuthResult result) {
    agent_.set_auth_result(result);
    ConnectAndWait();

    EXPECT_EQ(observer_.unavailable_count(), 1);
    EXPECT_EQ(observer_.last_unavailable_result(), result);
    EXPECT_EQ(observer_.connected_count(), 0);
    EXPECT_EQ(client_->state(), AgentClient::State::kUnavailable);
    EXPECT_FALSE(client_->browser_host());

    // No retry, however long we wait or however often we ask.
    const int attempts = agent_.connect_attempts();
    client_->EnsureConnected();
    task_environment_.FastForwardBy(kTimePastEveryRetry);
    EXPECT_EQ(agent_.connect_attempts(), attempts);
    EXPECT_EQ(client_->state(), AgentClient::State::kUnavailable);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  FakeAgent agent_;
  TestObserver observer_;
  std::unique_ptr<AgentClient> client_;
};

// Constructing a client must not reach for the agent, or every profile would
// wake it at startup.
TEST_F(AgentClientTest, InitiallyIdle) {
  EXPECT_EQ(client_->state(), AgentClient::State::kDisconnected);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_EQ(agent_.connect_attempts(), 0);
}

TEST_F(AgentClientTest, HandshakeSucceeds) {
  ConnectAndWait();

  EXPECT_EQ(observer_.connected_count(), 1);
  EXPECT_EQ(observer_.not_running_count(), 0);
  EXPECT_EQ(observer_.unavailable_count(), 0);

  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
  EXPECT_TRUE(client_->is_connected());
  EXPECT_TRUE(client_->browser_host());

  EXPECT_EQ(agent_.connect_attempts(), 1);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 1);
  EXPECT_EQ(agent_.last_protocol_version(), mojom::kProtocolVersion);
  EXPECT_TRUE(agent_.has_browser_endpoint());
}

TEST_F(AgentClientTest, EnsureConnectedIsIdempotent) {
  client_->EnsureConnected();
  client_->EnsureConnected();
  client_->EnsureConnected();
  WaitForNotification();

  EXPECT_EQ(observer_.connected_count(), 1);
  EXPECT_EQ(agent_.connect_attempts(), 1);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 1);
}

TEST_F(AgentClientTest, LosingSessionPipesWhileConnectedReconnects) {
  ConnectAndWait();

  agent_.DropSessionHandles();
  WaitForNotification();

  EXPECT_EQ(observer_.disconnected_count(), 1);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
}

// The host pipe is bound before the handshake completes, so the state is what
// gates access to it, not the remote.
TEST_F(AgentClientTest, HostIsNotPublishedBeforeAcceptance) {
  agent_.set_auth_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);

  ASSERT_TRUE(agent_.has_held_request());
  EXPECT_EQ(client_->state(), AgentClient::State::kConnecting);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_EQ(observer_.connected_count(), 0);
}

TEST_F(AgentClientTest, UnreachableAgentIsRetriedUntilItAppears) {
  agent_.set_transport_fails(true);
  ConnectAndWait();

  EXPECT_EQ(observer_.not_running_count(), 1);
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
  EXPECT_FALSE(client_->browser_host());
  // Nothing was ever up, so there is no session loss to report.
  EXPECT_EQ(observer_.disconnected_count(), 0);

  agent_.set_transport_fails(false);
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  EXPECT_GT(agent_.connect_attempts(), 1);
  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
  EXPECT_EQ(observer_.connected_count(), 1);
}

// The service reacts to this by trying to start the agent, so it has to be one
// notification per run of failures rather than one per attempt.
TEST_F(AgentClientTest, NotRunningIsNotifiedOncePerRunOfFailures) {
  agent_.set_transport_fails(true);
  ConnectAndWait();
  ASSERT_EQ(observer_.not_running_count(), 1);

  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_GT(agent_.connect_attempts(), 2);
  EXPECT_EQ(observer_.not_running_count(), 1);

  // A success ends the run; the next failure after it is a new one.
  agent_.set_transport_fails(false);
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  agent_.set_transport_fails(true);
  agent_.CloseAllConnections();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(observer_.not_running_count(), 2);
}

TEST_F(AgentClientTest, RetryDelayGrowsWhileFailing) {
  agent_.set_transport_fails(true);
  ConnectAndWait();

  const int before_first_window = agent_.connect_attempts();
  task_environment_.FastForwardBy(kTimeBetweenRetryWindows);
  const int first_window = agent_.connect_attempts() - before_first_window;

  task_environment_.FastForwardBy(kTimeBetweenRetryWindows);
  const int second_window =
      agent_.connect_attempts() - first_window - before_first_window;

  EXPECT_GT(first_window, 0);
  EXPECT_GT(first_window, second_window);
}

// A session that was up and then dropped is not evidence that reconnecting will
// fail, so the ramp starts over rather than continuing from where it was.
TEST_F(AgentClientTest, LosingSessionResetsTheBackoff) {
  // Climb the ramp first, so that inheriting it would be visible.
  agent_.set_transport_fails(true);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_GT(agent_.connect_attempts(), 2);

  agent_.set_transport_fails(false);
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  // Drop the session and make the next attempt fail, so the client stops in
  // kWaitingToRetry with a delay we can time.
  agent_.set_transport_fails(true);
  agent_.CloseAllConnections();
  WaitForNotification();
  ASSERT_EQ(observer_.disconnected_count(), 1);
  ASSERT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);

  // Well under the capped delay the old ramp had reached, but over the first
  // delay of a fresh one.
  const int attempts = agent_.connect_attempts();
  task_environment_.FastForwardBy(base::Seconds(2));
  EXPECT_GT(agent_.connect_attempts(), attempts);
}

TEST_F(AgentClientTest, RejectionIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserAuthResult::kRejected);
}

TEST_F(AgentClientTest, VersionMismatchIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserAuthResult::kVersionMismatch);
}

TEST_F(AgentClientTest, HostAlreadyRequestedIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserAuthResult::kHostAlreadyRequested);
}

// The reason the provider pipe is held open in the terminal state: the verdict
// belongs to that agent binary, so its replacement gets a fresh answer. This is
// how a browser that updated ahead of a running agent recovers without being
// restarted.
TEST_F(AgentClientTest, RefusedClientReconnectsWhenAgentIsReplaced) {
  agent_.set_auth_result(mojom::BrowserAuthResult::kVersionMismatch);
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kUnavailable);

  // The refusing agent exits and a newer one takes its place.
  agent_.set_auth_result(mojom::BrowserAuthResult::kAccepted);
  agent_.CloseAllConnections();
  WaitForNotification();

  EXPECT_EQ(observer_.connected_count(), 1);
  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
  EXPECT_TRUE(client_->browser_host());
}

TEST_F(AgentClientTest, NoServerNameIsTerminalUntilReset) {
  agent_.set_server_name_available(false);
  ConnectAndWait();

  EXPECT_EQ(observer_.unavailable_count(), 1);
  // Not an auth result: there was no agent to hear from.
  EXPECT_EQ(observer_.last_unavailable_result(), std::nullopt);
  EXPECT_EQ(client_->state(), AgentClient::State::kUnavailable);
  EXPECT_EQ(agent_.connect_attempts(), 0);

  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(agent_.connect_attempts(), 0);

  // Reset() is the only way out, and there is nothing holding the state open.
  client_->Reset();
  EXPECT_EQ(client_->state(), AgentClient::State::kDisconnected);

  agent_.set_server_name_available(true);
  ConnectAndWait();
  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
}

// A peer that takes the connection and never answers, shouldn't hold the client
// in kConnecting forever.
TEST_F(AgentClientTest, SilentAgentTimesOutAndRetries) {
  agent_.set_auth_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);
  ASSERT_EQ(client_->state(), AgentClient::State::kConnecting);

  task_environment_.FastForwardBy(kTimePastEveryRetry);

  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_GT(agent_.connect_attempts(), 1);
}

// The reply travels on the provider pipe and the refusal drops handles on two
// others, with no ordering between them. An acceptance that lands after the
// session pipes are gone must not publish a host that cannot deliver anything.
TEST_F(AgentClientTest, AcceptanceOnDeadSessionIsNotPublished) {
  agent_.set_auth_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);
  ASSERT_TRUE(agent_.has_held_request());

  // The client sees the pipes close first.
  agent_.DropSessionHandles();
  task_environment_.FastForwardBy(kTimeBriefly);
  agent_.AnswerHeldRequest(mojom::BrowserAuthResult::kAccepted);
  task_environment_.FastForwardBy(kTimeBriefly);

  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
}

TEST_F(AgentClientTest, ResetWhileConnectingIsSafe) {
  client_->EnsureConnected();
  client_->Reset();

  base::ThreadPoolInstance::Get()->FlushForTesting();
  task_environment_.FastForwardBy(kTimeBriefly);

  EXPECT_EQ(client_->state(), AgentClient::State::kDisconnected);
  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_EQ(observer_.disconnected_count(), 0);

  ConnectAndWait();
  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
}

TEST_F(AgentClientTest, ResetWhileConnectedNotifiesAndSchedulesNothing) {
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  client_->Reset();

  EXPECT_EQ(observer_.disconnected_count(), 1);
  EXPECT_EQ(client_->state(), AgentClient::State::kDisconnected);
  EXPECT_FALSE(client_->browser_host());

  const int attempts = agent_.connect_attempts();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(agent_.connect_attempts(), attempts);
}

// Several clients per browser process is the expected arrangement, since the
// agent keys sessions by connection.
TEST_F(AgentClientTest, TwoClientsGetIndependentSessions) {
  TestObserver second_observer;
  std::unique_ptr<AgentClient> second_client = CreateClient(&second_observer);

  ConnectAndWait();
  second_client->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);

  EXPECT_TRUE(client_->is_connected());
  EXPECT_TRUE(second_client->is_connected());
  EXPECT_EQ(agent_.connection_count(), 2u);
  EXPECT_EQ(agent_.session_count(), 2u);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 2);

  // One going away leaves the other alone.
  second_client->Reset();
  task_environment_.FastForwardBy(kTimeBriefly);
  EXPECT_TRUE(client_->is_connected());
  EXPECT_EQ(agent_.connection_count(), 1u);

  second_client->RemoveObserver(&second_observer);
}

}  // namespace brave_vpn::v2
