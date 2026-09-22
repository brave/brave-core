/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_client.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/callback.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/agent/test/fake_agent.h"
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

// Time span in which the failing client is still under the persistent
// threshold.
constexpr base::TimeDelta kTimeBeforePersistentFailure = base::Seconds(12);

// Time spans of either side of the minimum stable session, which decides
// whether a dropped session ends a run of failures or counts as another failure
// in it.
constexpr base::TimeDelta kTimeShorterThanStableSession = base::Seconds(1);
constexpr base::TimeDelta kTimeLongerThanStableSession = base::Seconds(4);

// A first retry delay is at most the backoff's initial delay, so anything
// higher than this can only come from a run of failures that wasn't cleared.
constexpr base::TimeDelta kLongestFirstRetryDelay = base::Seconds(1);

class TestObserver : public AgentClient::Observer {
 public:
  void OnAgentConnected() override {
    ++connected_count_;
    Notify();
  }

  void OnAgentSessionStable() override {
    ++session_stable_count_;
    Notify();
  }

  void OnAgentDisconnected() override {
    ++disconnected_count_;
    Notify();
  }

  void OnAgentConnectionFailed(AgentClient::Error error) override {
    ++failure_count_;
    last_connection_error_ = error;
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
  int session_stable_count() const { return session_stable_count_; }
  int disconnected_count() const { return disconnected_count_; }
  int failure_count() const { return failure_count_; }
  int not_running_count() const { return not_running_count_; }
  std::optional<AgentClient::Error> last_connection_error() const {
    return last_connection_error_;
  }

 private:
  void Notify() {
    if (on_notification_) {
      on_notification_.Run();
    }
  }

  int connected_count_ = 0;
  int disconnected_count_ = 0;
  int failure_count_ = 0;
  int session_stable_count_ = 0;
  int not_running_count_ = 0;
  std::optional<AgentClient::Error> last_connection_error_;
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

  // Advances just far enough for the scheduled retry to run and the connect it
  // starts to settle.
  void RunNextScheduledRetry() {
    const base::TimeDelta delay =
        task_environment_.NextMainThreadPendingTaskDelay();
    ASSERT_NE(delay, base::TimeDelta::Max());
    task_environment_.FastForwardBy(delay + kTimeBriefly);
  }

  // Every refusal is terminal in the same way, whatever the agent's reason and
  // whichever call it came from.
  void ExpectRefusalIsTerminal(mojom::BrowserInitResult result,
                               AgentClient::Error expected_error) {
    agent_.set_init_result(result);
    ExpectTerminalRefusalAfterConnect(expected_error);
  }

  void ExpectRefusalIsTerminal(mojom::BrowserAuthResult result,
                               AgentClient::Error expected_error) {
    agent_.set_auth_result(result);
    ExpectTerminalRefusalAfterConnect(expected_error);
  }

  void ExpectRefusalIsTerminal(AgentIdentity::VerificationResult result,
                               AgentClient::Error expected_error) {
    agent_.set_identity_result(result);
    ExpectTerminalRefusalAfterConnect(expected_error);
  }

  void ExpectTerminalRefusalAfterConnect(AgentClient::Error expected_error) {
    ConnectAndWait();

    EXPECT_EQ(observer_.failure_count(), 1);
    EXPECT_EQ(observer_.last_connection_error(), expected_error);
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
  EXPECT_EQ(observer_.failure_count(), 0);

  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
  EXPECT_TRUE(client_->is_connected());
  EXPECT_TRUE(client_->browser_host());

  EXPECT_EQ(agent_.connect_attempts(), 1);
  EXPECT_EQ(agent_.initialize_calls(), 1);
  EXPECT_EQ(agent_.last_protocol_version(), mojom::kProtocolVersion);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 1);
  EXPECT_TRUE(agent_.has_browser_endpoint());
}

TEST_F(AgentClientTest, EnsureConnectedIsIdempotent) {
  client_->EnsureConnected();
  client_->EnsureConnected();
  client_->EnsureConnected();
  WaitForNotification();

  EXPECT_EQ(observer_.connected_count(), 1);
  EXPECT_EQ(agent_.connect_attempts(), 1);
  EXPECT_EQ(agent_.initialize_calls(), 1);
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

  ASSERT_TRUE(agent_.has_held_auth_request());
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

  // An outage that resolved on its own is not worth telling anyone about.
  EXPECT_EQ(observer_.failure_count(), 0);
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

  EXPECT_EQ(observer_.session_stable_count(), 1);
  EXPECT_EQ(observer_.not_running_count(), 2);
}

// Reaching the agent answers an outstanding launch request, even if the
// session it established is too short to end the run of failures. Otherwise an
// agent that dies right after connecting would be started once and then left
// alone for the rest of the run.
TEST_F(AgentClientTest, ShortSessionStillAllowsAnotherLaunchRequest) {
  agent_.set_transport_fails(true);
  ConnectAndWait();
  ASSERT_EQ(observer_.not_running_count(), 1);

  agent_.set_transport_fails(false);
  RunNextScheduledRetry();
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  task_environment_.FastForwardBy(kTimeShorterThanStableSession);
  agent_.set_transport_fails(true);
  agent_.CloseAllConnections();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  EXPECT_EQ(observer_.session_stable_count(), 0);
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

// A run of failures ends once a session has lasted kMinStableSession, so a ramp
// climbed before connecting is not inherited by the failures that follow it.
// The drop is not what clears it: one that came sooner would've counted as
// another failure in the same run.
TEST_F(AgentClientTest, ConnectedSessionResetsTheBackoff) {
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

// A session that ran for a while and then dropped is not evidence that
// reconnecting will fail, so each drop starts the ramp over.
TEST_F(AgentClientTest, StableSessionsKeepTheBackoffAtItsFirstDelay) {
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  for (int i = 0; i < 3; ++i) {
    task_environment_.FastForwardBy(kTimeLongerThanStableSession);
    agent_.CloseAllConnections();
    WaitForNotification();
    ASSERT_EQ(client_->state(), AgentClient::State::kWaitingToRetry)
        << "drop " << i;

    EXPECT_LE(task_environment_.NextMainThreadPendingTaskDelay(),
              kLongestFirstRetryDelay)
        << "drop " << i;

    RunNextScheduledRetry();
    ASSERT_EQ(client_->state(), AgentClient::State::kConnected)
        << "reconnect " << i;
  }
}

// The opposite case, and the one an agent that crashes on startup produces:
// sessions that die immediately are part of the run of failures rather than
// evidence against it. Were they to clear it, the client would reconnect on
// the initial delay forever and never report anything.
TEST_F(AgentClientTest, ShortSessionsDoNotResetTheBackoff) {
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  constexpr int kShortSessions = 5;
  for (int i = 0; i < kShortSessions; ++i) {
    task_environment_.FastForwardBy(kTimeShorterThanStableSession);
    agent_.CloseAllConnections();
    WaitForNotification();
    ASSERT_EQ(client_->state(), AgentClient::State::kWaitingToRetry)
        << "drop " << i;

    if (i + 1 == kShortSessions) {
      break;
    }
    RunNextScheduledRetry();
    ASSERT_EQ(client_->state(), AgentClient::State::kConnected)
        << "reconnect " << i;
  }

  // Five failures in one run puts the next delay well past a first delay,
  // whose ceiling is the backoff's initial delay.
  EXPECT_GT(task_environment_.NextMainThreadPendingTaskDelay(),
            kLongestFirstRetryDelay);

  // Every session died inside its window, so none of them ever counted.
  EXPECT_EQ(observer_.session_stable_count(), 0);
}

// A session lost inside a crash loop's stability window is an agent that cannot
// stay up rather than one that went quiet, so that is the verdict the customer
// gets, even though what the client observed was a dropped pipe.
TEST_F(AgentClientTest, CrashLoopIsReportedAsUnstable) {
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  constexpr int kShortSessions = 30;
  int cycles = 0;
  for (; cycles < kShortSessions; ++cycles) {
    task_environment_.FastForwardBy(kTimeShorterThanStableSession);
    agent_.CloseAllConnections();
    WaitForNotification();
    ASSERT_EQ(client_->state(), AgentClient::State::kWaitingToRetry)
        << "drop " << cycles;

    if (observer_.failure_count() > 0) {
      break;
    }
    RunNextScheduledRetry();
    ASSERT_EQ(client_->state(), AgentClient::State::kConnected)
        << "reconnect " << cycles;
  }
  ASSERT_LT(cycles, kShortSessions) << "the run of failures was never reported";

  EXPECT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentUnstable);
  // Every session died inside its window, so none of them ever counted.
  EXPECT_EQ(observer_.session_stable_count(), 0);
}

TEST_F(AgentClientTest, VersionMismatchIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserInitResult::kVersionMismatch,
                          AgentClient::Error::kBrowserRejected);
}

TEST_F(AgentClientTest, InitInvalidRequestIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserInitResult::kInvalidRequest,
                          AgentClient::Error::kUnexpectedBehavior);
}

TEST_F(AgentClientTest, RejectionIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserAuthResult::kRejected,
                          AgentClient::Error::kBrowserRejected);
}

TEST_F(AgentClientTest, HostAlreadyRequestedIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserAuthResult::kHostAlreadyRequested,
                          AgentClient::Error::kUnexpectedBehavior);
}

TEST_F(AgentClientTest, InvalidRequestIsTerminal) {
  ExpectRefusalIsTerminal(mojom::BrowserAuthResult::kInvalidRequest,
                          AgentClient::Error::kUnexpectedBehavior);
}

TEST_F(AgentClientTest, AgentRejectedIsTerminal) {
  ExpectRefusalIsTerminal(AgentIdentity::VerificationResult::kRejected,
                          AgentClient::Error::kUnexpectedBehavior);
}

// The reason the provider pipe is held open in the terminal state: the verdict
// belongs to that agent binary, so its replacement gets a fresh answer. This is
// how a browser that updated ahead of a running agent recovers without being
// restarted.
TEST_F(AgentClientTest, RefusedClientReconnectsWhenAgentIsReplaced) {
  agent_.set_init_result(mojom::BrowserInitResult::kVersionMismatch);
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kUnavailable);

  // The refusing agent exits and a newer one takes its place.
  agent_.set_init_result(mojom::BrowserInitResult::kInitialized);
  agent_.CloseAllConnections();
  WaitForNotification();

  EXPECT_EQ(observer_.connected_count(), 1);
  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
  EXPECT_TRUE(client_->browser_host());
}

TEST_F(AgentClientTest, NoServerNameIsTerminalUntilReset) {
  agent_.set_server_name_available(false);
  ConnectAndWait();

  EXPECT_EQ(observer_.failure_count(), 1);
  // Not an auth result: there was no agent to hear from.
  EXPECT_EQ(observer_.last_connection_error(), AgentClient::Error::kNoEndpoint);
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

// Retryable errors are reported only once a run of them has lasted long
// enough to stop looking like a blip, and then only once.
TEST_F(AgentClientTest, PersistentFailureIsReportedOncePerRun) {
  agent_.set_transport_fails(true);
  ConnectAndWait();

  // Several failures, but not yet long enough to be a verdict.
  task_environment_.FastForwardBy(kTimeBeforePersistentFailure);
  ASSERT_GT(agent_.connect_attempts(), 1);
  EXPECT_EQ(observer_.failure_count(), 0);

  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentUnreachable);
  // Reporting is not a state change: the client is still trying.
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);

  // However long the run goes on, it is the same news.
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(observer_.failure_count(), 1);

  // A success ends the run, so the next one is reportable again.
  agent_.set_transport_fails(false);
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(client_->state(), AgentClient::State::kConnected);

  agent_.set_transport_fails(true);
  agent_.CloseAllConnections();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(observer_.failure_count(), 2);
}

// The case nothing else reports: a peer holds the endpoint and never answers.
// The transport connects, so the agent is never treated as missing and no
// launch is asked for - correctly, since starting the agent cannot take an
// endpoint another process is already holding.
TEST_F(AgentClientTest, SilentPeerIsReportedWithoutBeingTreatedAsMissing) {
  agent_.set_auth_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);
  ASSERT_EQ(client_->state(), AgentClient::State::kConnecting);

  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentNotResponding);
  EXPECT_EQ(observer_.not_running_count(), 0);
  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_GT(agent_.connect_attempts(), 1);
}

// An inconclusive verdict is retried rather than treated as a refusal, but a
// run of them still has to surface.
TEST_F(AgentClientTest, RepeatedInconclusiveResultsAreReportedAndRetried) {
  agent_.set_auth_result(mojom::BrowserAuthResult::kInconclusive);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kBrowserUnverified);
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
  EXPECT_GT(agent_.connect_attempts(), 1);
}

// A refused Initialize() has to stop the handshake, not merely colour its
// result: asking for a host afterwards is what the agent answers with
// kInvalidRequest.
TEST_F(AgentClientTest, HostIsNotRequestedWhenInitializeIsRefused) {
  agent_.set_init_result(mojom::BrowserInitResult::kVersionMismatch);
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kUnavailable);

  EXPECT_EQ(agent_.initialize_calls(), 1);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 0);
  EXPECT_EQ(agent_.session_count(), 0u);
}

// Verification runs between the two handshake calls, and an agent that fails it
// is never handed a BrowserEndpoint.
TEST_F(AgentClientTest, HostIsNotRequestedWhenAgentVerificationFails) {
  agent_.set_identity_result(AgentIdentity::VerificationResult::kRejected);
  ConnectAndWait();
  ASSERT_EQ(client_->state(), AgentClient::State::kUnavailable);

  EXPECT_EQ(agent_.initialize_calls(), 1);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 0);
  EXPECT_EQ(agent_.session_count(), 0u);
}

// Being unable to identify the connection is not a verdict about this binary,
// and only a new connection can produce a fresh capture, so it is retried
// rather than treated as a refusal. A run of them still has to surface.
TEST_F(AgentClientTest, RepeatedNotIdentifiedResultsAreReportedAndRetried) {
  agent_.set_init_result(mojom::BrowserInitResult::kNotIdentified);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kBrowserUnverified);
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
  EXPECT_GT(agent_.connect_attempts(), 1);
}

// No identity message, an image replaced mid-update, a rotated cert: not a
// verdict, so it is retried. A run of them still has to surface.
TEST_F(AgentClientTest, RepeatedInconclusiveAgentVerificationIsRetried) {
  agent_.set_identity_result(AgentIdentity::VerificationResult::kInconclusive);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentNotResponding);
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
  EXPECT_GT(agent_.connect_attempts(), 1);
}

// The transport comes up but the peer cannot be pinned, so there is nothing to
// verify against and the handshake never starts. Local and likely transient,
// so it is retried, and it recovers on its own once capture works again.
TEST_F(AgentClientTest, IdentityCaptureFailureIsRetried) {
  agent_.set_identity_result(std::nullopt);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentNotResponding);
  EXPECT_GT(agent_.connect_attempts(), 1);
  // The distinguishing assertion: this path fails before Initialize() is sent.
  EXPECT_EQ(agent_.initialize_calls(), 0);

  agent_.set_identity_result(AgentIdentity::VerificationResult::kAccepted);
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(client_->state(), AgentClient::State::kConnected);
}

// The handshake timeout covers Initialize() too, so a peer that takes the
// connection and never answers the first call is treated the same as one that
// goes quiet on the second.
TEST_F(AgentClientTest, SilentInitializeIsReportedAsNotResponding) {
  agent_.set_init_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);
  ASSERT_TRUE(agent_.has_held_init_request());
  ASSERT_EQ(client_->state(), AgentClient::State::kConnecting);
  EXPECT_EQ(agent_.bind_browser_host_calls(), 0);

  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentNotResponding);
  EXPECT_EQ(observer_.not_running_count(), 0);
  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_GT(agent_.connect_attempts(), 1);
}

TEST_F(AgentClientTest, ResetClearsTheReportedError) {
  agent_.set_transport_fails(true);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(observer_.failure_count(), 1);

  client_->Reset();
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  EXPECT_EQ(observer_.failure_count(), 2);
}

// Acceptance is not evidence that connecting works; staying connected is.
TEST_F(AgentClientTest, SessionIsReportedStableOnlyAfterItLasted) {
  ConnectAndWait();
  ASSERT_EQ(observer_.connected_count(), 1);
  EXPECT_EQ(observer_.session_stable_count(), 0);

  task_environment_.FastForwardBy(kTimeShorterThanStableSession);
  EXPECT_EQ(observer_.session_stable_count(), 0);

  task_environment_.FastForwardBy(kTimeLongerThanStableSession);
  EXPECT_EQ(observer_.session_stable_count(), 1);
}

TEST_F(AgentClientTest, SessionLostBeforeStabilityIsNeverReportedStable) {
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimeShorterThanStableSession);

  // Nothing to reconnect to, so the client cannot quietly establish a second
  // session that would become stable on its own.
  agent_.set_transport_fails(true);
  agent_.CloseAllConnections();
  WaitForNotification();
  ASSERT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);

  task_environment_.FastForwardBy(kTimePastEveryRetry);
  EXPECT_EQ(observer_.session_stable_count(), 0);
}

// A run of failures that shifts between retryable reasons is still one piece
// of news. Reporting each turn would churn whatever the owner puts in front of
// the customer - an agent crash loop alternates on every cycle.
TEST_F(AgentClientTest, ShiftingRetryableReasonsAreReportedOnce) {
  agent_.set_transport_fails(true);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(observer_.failure_count(), 1);
  ASSERT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentUnreachable);

  // The agent spins up, but now holds the connection without answering: a
  // different retryable reason within the same run.
  agent_.set_transport_fails(false);
  agent_.set_auth_result(std::nullopt);
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_EQ(observer_.connected_count(), 0);
  EXPECT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentUnreachable);
}

// A verdict retrying cannot change has to reach the customer even though
// something already has, because the advice it carries is different.
TEST_F(AgentClientTest, EscalationToTerminalReasonIsStillReported) {
  agent_.set_transport_fails(true);
  ConnectAndWait();
  task_environment_.FastForwardBy(kTimePastEveryRetry);
  ASSERT_EQ(observer_.failure_count(), 1);
  ASSERT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentUnreachable);

  agent_.set_transport_fails(false);
  agent_.set_auth_result(mojom::BrowserAuthResult::kRejected);
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  EXPECT_EQ(observer_.failure_count(), 2);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kBrowserRejected);
  EXPECT_EQ(client_->state(), AgentClient::State::kUnavailable);
}

// A verdict belongs to the agent binary that gave it, so losing that agent
// clears the run and its replacement is asked afresh. If the replacement
// refuses too, that is news again rather than a repeat: the previous answer
// was about a process that no longer exists.
TEST_F(AgentClientTest, ReplacementAgentRefusingAgainIsReported) {
  agent_.set_auth_result(mojom::BrowserAuthResult::kRejected);
  ConnectAndWait();
  ASSERT_EQ(observer_.failure_count(), 1);

  agent_.CloseAllConnections();
  task_environment_.FastForwardBy(kTimePastEveryRetry);

  ASSERT_GT(agent_.connect_attempts(), 1);
  EXPECT_EQ(observer_.failure_count(), 2);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kBrowserRejected);
  EXPECT_EQ(client_->state(), AgentClient::State::kUnavailable);
}

// The reply travels on the provider pipe and the refusal drops handles on two
// others, with no ordering between them. An acceptance that lands after the
// session pipes are gone must not publish a host that cannot deliver anything.
TEST_F(AgentClientTest, AcceptanceOnDeadSessionIsNotPublished) {
  agent_.set_auth_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);
  ASSERT_TRUE(agent_.has_held_auth_request());

  // The client sees the pipes close first.
  agent_.DropSessionHandles();
  task_environment_.FastForwardBy(kTimeBriefly);
  agent_.AnswerHeldAuthRequest(mojom::BrowserAuthResult::kAccepted);
  task_environment_.FastForwardBy(kTimeBriefly);

  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_FALSE(client_->browser_host());
  EXPECT_EQ(client_->state(), AgentClient::State::kWaitingToRetry);
}

// An acceptance arriving on a dead session is scored the same way a session
// lost inside its stability window is, so once the run has lasted long enough
// that is the verdict reported.
TEST_F(AgentClientTest, RepeatedAcceptanceOnDeadSessionIsReportedAsUnstable) {
  agent_.set_auth_result(std::nullopt);
  client_->EnsureConnected();
  task_environment_.FastForwardBy(kTimeBriefly);

  constexpr int kMaxSessions = 30;
  int races = 0;
  for (; races < kMaxSessions; ++races) {
    ASSERT_TRUE(agent_.has_held_auth_request()) << "race " << races;
    agent_.DropSessionHandles();
    task_environment_.FastForwardBy(kTimeBriefly);
    agent_.AnswerHeldAuthRequest(mojom::BrowserAuthResult::kAccepted);
    task_environment_.FastForwardBy(kTimeBriefly);
    ASSERT_EQ(client_->state(), AgentClient::State::kWaitingToRetry)
        << "race " << races;

    if (observer_.failure_count() > 0) {
      break;
    }
    RunNextScheduledRetry();
    ASSERT_EQ(client_->state(), AgentClient::State::kConnecting)
        << "reconnect " << races;
  }
  ASSERT_LT(races, kMaxSessions) << "the run of failures was never reported";

  EXPECT_EQ(observer_.failure_count(), 1);
  EXPECT_EQ(observer_.last_connection_error(),
            AgentClient::Error::kAgentUnstable);
  // No host was ever published, so there was nothing to lose either.
  EXPECT_EQ(observer_.connected_count(), 0);
  EXPECT_EQ(observer_.disconnected_count(), 0);
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
