/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/app/v2/agent/connection_api.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_api.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_manager.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// Records every notification it receives, oldest first.
class TestObserver : public ConnectionManager::Observer {
 public:
  struct ConnectionStateUpdate {
    mojom::VpnConnectionState state;
    std::optional<std::string> error_message;
  };

  struct AllowLanTrafficStateUpdate {
    bool available = false;
    bool enabled = false;
  };

  explicit TestObserver(ConnectionManager& manager) {
    observation_.Observe(&manager);
  }
  ~TestObserver() override = default;

  TestObserver(const TestObserver&) = delete;
  TestObserver& operator=(const TestObserver&) = delete;

  // ConnectionManager::Observer:
  void OnConnectionStateChanged(
      mojom::VpnConnectionState state,
      std::optional<std::string> error_message) override {
    connection_state_updates_.push_back(
        ConnectionStateUpdate{.state = state, .error_message = error_message});
    if (quit_on_awaited_state_ && state == awaited_state_) {
      std::move(quit_on_awaited_state_).Run();
    }
  }
  void OnSelectedRegionChanged(const mojom::VpnRegion& region) override {
    selected_region_name_updates_.push_back(region.name);
  }
  void OnAutoReconnectStateChanged(bool enabled) override {
    auto_reconnect_state_updates_.push_back(enabled);
  }
  void OnSmartProxyRoutingStateChanged(bool enabled) override {
    smart_proxy_routing_state_updates_.push_back(enabled);
  }
  void OnAllowLanTrafficStateChanged(bool available, bool enabled) override {
    allow_lan_traffic_state_updates_.push_back(
        AllowLanTrafficStateUpdate{.available = available, .enabled = enabled});
  }
  void OnLastHostnameChanged(const std::string& hostname) override {
    last_hostname_updates_.push_back(hostname);
  }

  const std::vector<ConnectionStateUpdate>& connection_state_updates() const {
    return connection_state_updates_;
  }
  const std::vector<std::string>& selected_region_name_updates() const {
    return selected_region_name_updates_;
  }
  const std::vector<bool>& auto_reconnect_state_updates() const {
    return auto_reconnect_state_updates_;
  }
  const std::vector<bool>& smart_proxy_routing_state_updates() const {
    return smart_proxy_routing_state_updates_;
  }
  const std::vector<AllowLanTrafficStateUpdate>&
  allow_lan_traffic_state_updates() const {
    return allow_lan_traffic_state_updates_;
  }
  const std::vector<std::string>& last_hostname_updates() const {
    return last_hostname_updates_;
  }

  // Runs a RunLoop until |state| is reported. Only a report that arrives
  // during the wait counts.
  void WaitForConnectionState(mojom::VpnConnectionState state) {
    base::RunLoop run_loop;
    awaited_state_ = state;
    quit_on_awaited_state_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  std::vector<ConnectionStateUpdate> connection_state_updates_;
  mojom::VpnConnectionState awaited_state_ =
      mojom::VpnConnectionState::kDisconnected;
  base::OnceClosure quit_on_awaited_state_;
  std::vector<std::string> selected_region_name_updates_;
  std::vector<bool> auto_reconnect_state_updates_;
  std::vector<bool> smart_proxy_routing_state_updates_;
  std::vector<AllowLanTrafficStateUpdate> allow_lan_traffic_state_updates_;
  std::vector<std::string> last_hostname_updates_;
  base::ScopedObservation<ConnectionManager, ConnectionManager::Observer>
      observation_{this};
};

// Smaller than every delay the connection manager uses; steps the clock to just
// before or just after one of them.
constexpr base::TimeDelta kSessionTick = base::Milliseconds(100);
// Long enough for anything the connection manager might still have scheduled to
// happen.
constexpr base::TimeDelta kSessionLongTime = base::Minutes(10);

}  // namespace

// The state handling under test lives in ConnectionManager itself; the fake
// only exposes its Update*() methods, so these tests exercise the real logic.
class ConnectionManagerTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  FakeConnectionManager manager_;
  TestObserver observer_{manager_};
};

TEST_F(ConnectionManagerTest, UpdatesConnectionState) {
  EXPECT_EQ(mojom::VpnConnectionState::kDisconnected,
            manager_.connection_state());
  EXPECT_EQ(std::nullopt, manager_.error_message());

  // Same as the initial state.
  manager_.UpdateConnectionState(mojom::VpnConnectionState::kDisconnected,
                                 std::nullopt);
  manager_.UpdateConnectionState(mojom::VpnConnectionState::kConnectFailed,
                                 "Server unreachable");
  manager_.UpdateConnectionState(mojom::VpnConnectionState::kConnectFailed,
                                 "Server unreachable");

  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed,
            manager_.connection_state());
  EXPECT_EQ("Server unreachable", manager_.error_message());

  const auto& updates = observer_.connection_state_updates();
  ASSERT_EQ(1u, updates.size());
  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed, updates[0].state);
  EXPECT_EQ("Server unreachable", updates[0].error_message);
}

// The error message counts on its own: replacing or clearing it at the same
// state must notify, so observers never keep showing a stale message.
TEST_F(ConnectionManagerTest, UpdatesErrorMessageAtSameConnectionState) {
  manager_.UpdateConnectionState(mojom::VpnConnectionState::kConnectFailed,
                                 "Server unreachable");
  manager_.UpdateConnectionState(mojom::VpnConnectionState::kConnectFailed,
                                 "Handshake timed out");
  manager_.UpdateConnectionState(mojom::VpnConnectionState::kConnectFailed,
                                 std::nullopt);

  const auto& updates = observer_.connection_state_updates();
  ASSERT_EQ(3u, updates.size());
  EXPECT_EQ("Handshake timed out", updates[1].error_message);
  EXPECT_EQ(std::nullopt, updates[2].error_message);
  EXPECT_EQ(std::nullopt, manager_.error_message());
}

// Regions are compared by value, not by pointer: a new but equal region is no
// change.
TEST_F(ConnectionManagerTest, UpdatesSelectedRegion) {
  EXPECT_EQ("", manager_.current_region().name);

  manager_.UpdateSelectedRegion(mojom::VpnRegion::New("Europe"));
  manager_.UpdateSelectedRegion(mojom::VpnRegion::New("Europe"));
  manager_.UpdateSelectedRegion(mojom::VpnRegion::New("Asia"));

  EXPECT_EQ("Asia", manager_.current_region().name);
  EXPECT_EQ((std::vector<std::string>{"Europe", "Asia"}),
            observer_.selected_region_name_updates());
}

// Both fields count: a change to either one alone must notify.
TEST_F(ConnectionManagerTest, UpdatesAllowLanTrafficState) {
  EXPECT_FALSE(manager_.allow_lan_traffic_available());
  EXPECT_FALSE(manager_.allow_lan_traffic_enabled());

  // Only |available| changes from the initial {false, false}.
  manager_.UpdateAllowLanTrafficState(true, false);
  // Only |enabled| changes.
  manager_.UpdateAllowLanTrafficState(true, true);
  manager_.UpdateAllowLanTrafficState(true, true);

  EXPECT_TRUE(manager_.allow_lan_traffic_available());
  EXPECT_TRUE(manager_.allow_lan_traffic_enabled());

  const auto& updates = observer_.allow_lan_traffic_state_updates();
  ASSERT_EQ(2u, updates.size());
  EXPECT_TRUE(updates[0].available);
  EXPECT_FALSE(updates[0].enabled);
  EXPECT_TRUE(updates[1].available);
  EXPECT_TRUE(updates[1].enabled);
}

TEST_F(ConnectionManagerTest, UpdatesAutoReconnectState) {
  EXPECT_FALSE(manager_.auto_reconnect_enabled());

  manager_.UpdateAutoReconnectState(true);
  manager_.UpdateAutoReconnectState(true);
  manager_.UpdateAutoReconnectState(false);

  EXPECT_FALSE(manager_.auto_reconnect_enabled());
  EXPECT_EQ((std::vector<bool>{true, false}),
            observer_.auto_reconnect_state_updates());
}

TEST_F(ConnectionManagerTest, UpdatesSmartProxyRoutingState) {
  EXPECT_FALSE(manager_.smart_proxy_routing_enabled());

  manager_.UpdateSmartProxyRoutingState(true);
  manager_.UpdateSmartProxyRoutingState(true);
  manager_.UpdateSmartProxyRoutingState(false);

  EXPECT_FALSE(manager_.smart_proxy_routing_enabled());
  EXPECT_EQ((std::vector<bool>{true, false}),
            observer_.smart_proxy_routing_state_updates());
}

TEST_F(ConnectionManagerTest, UpdatesLastHostname) {
  EXPECT_EQ("", manager_.last_hostname());

  manager_.UpdateLastHostname("eu-1.example.com");
  manager_.UpdateLastHostname("eu-1.example.com");
  manager_.UpdateLastHostname("as-1.example.com");

  EXPECT_EQ("as-1.example.com", manager_.last_hostname());
  EXPECT_EQ((std::vector<std::string>{"eu-1.example.com", "as-1.example.com"}),
            observer_.last_hostname_updates());
}

// Tests of the connection logic. They run the real ConnectionManager against
// a fake ConnectionApi, a mock clock and a mock network change notifier.
class ConnectionManagerSessionTest : public testing::Test {
 protected:
  using State = mojom::VpnConnectionState;
  using FailureReason = ConnectionApi::FailureReason;

  std::unique_ptr<ConnectionApi> CreateConnectionApi(
      ConnectionApi::Delegate* delegate) {
    return std::make_unique<FakeConnectionApi>(delegate, &api_log_);
  }

  // The live API instance; only valid while the manager has one.
  FakeConnectionApi& api() { return CHECK_DEREF(api_log_.current.get()); }

  State state() const { return manager_.connection_state(); }

  // Every state reported to observers so far, oldest first.
  std::vector<State> ObservedStates() const {
    std::vector<State> states;
    for (const auto& update : observer_.connection_state_updates()) {
      states.push_back(update.state);
    }
    return states;
  }

  void FastForwardBy(base::TimeDelta delta) {
    task_environment_.FastForwardBy(delta);
  }

  // Brings the tunnel up.
  void ConnectAndEstablish() {
    manager_.Connect();
    api().ReportEstablished();
  }

  base::TimeDelta MaxRetryDelay(int failures) {
    const auto& policy = ConnectionManager::kReconnectBackoffPolicy;
    const double delay_ms = policy.initial_delay_ms *
                            std::pow(policy.multiply_factor, failures - 1);
    return base::Milliseconds(
        std::min<double>(delay_ms, policy.maximum_backoff_ms));
  }

  base::TimeDelta MinRetryDelay(int failures) {
    return MaxRetryDelay(failures) *
           (1 - ConnectionManager::kReconnectBackoffPolicy.jitter_factor);
  }

  // Fails the attempt in flight for a reason that is worth retrying.
  void FailAttempt() { api().ReportFailed(FailureReason::kServerUnreachable); }

  // Fails the attempt in flight and waits for the retry that follows to
  // start. |failures| is the number of failures in a row this one makes.
  void FailAttemptAndWaitForRetry(int failures) {
    FailAttempt();
    FastForwardBy(MaxRetryDelay(failures));
  }

  // Reports a network change the way NetworkChangeNotifier does while there
  // is a network: CONNECTION_NONE first, then the current type.
  void NotifyNetworkChanged() {
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
        net::NetworkChangeNotifier::CONNECTION_NONE);
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
        net::NetworkChangeNotifier::CONNECTION_WIFI);
    DeliverNetworkChangeNotifications();
  }

  // Takes the network away for real.
  void GoOffline() {
    network_change_notifier_.mock_network_change_notifier()->SetConnectionType(
        net::NetworkChangeNotifier::CONNECTION_NONE);
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
        net::NetworkChangeNotifier::CONNECTION_NONE);
    DeliverNetworkChangeNotifications();
  }

  // Brings the network back.
  void GoOnline() {
    network_change_notifier_.mock_network_change_notifier()->SetConnectionType(
        net::NetworkChangeNotifier::CONNECTION_WIFI);
    NotifyNetworkChanged();
  }

  // Lets the network change notifications sent so far reach the manager.
  // NetworkChangeNotifier posts each of them as a task to this sequence, so a
  // task posted after them runs once they have all been delivered. The mock
  // clock doesn't move.
  void DeliverNetworkChangeNotifications() {
    base::RunLoop run_loop;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitClosure());
    run_loop.Run();
  }

  // Hands a network change notification to the manager right away. For tests
  // where it has to arrive before a task that is already pending; everything
  // else goes through the notifier.
  void DeliverNetworkChangeNow(
      net::NetworkChangeNotifier::ConnectionType type) {
    manager_.OnNetworkChanged(type);
  }

  // Waits for the attempt that a Connect() during the teardown asked for. It
  // starts from a task, but without any delay, so the mock clock must not move.
  void WaitForAttemptAfterTeardown() {
    const base::TimeTicks start = task_environment_.NowTicks();
    observer_.WaitForConnectionState(State::kConnecting);
    EXPECT_EQ(start, task_environment_.NowTicks());
  }

  static constexpr base::TimeDelta kNetworkSettleDelay =
      ConnectionManager::kNetworkSettleDelay;
  static constexpr base::TimeDelta kTransitionGracePeriod =
      ConnectionManager::kTransitionGracePeriod;
  static constexpr base::TimeDelta kStableConnectionPeriod =
      ConnectionManager::kStableConnectionPeriod;
  static constexpr base::TimeDelta kTransitionTimeout =
      ConnectionManager::kTransitionTimeout;
  static constexpr int kMaxReconnectAttempts =
      ConnectionManager::kMaxReconnectAttempts;

  // Relationships between the manager's timings that the tests rely on.
  static_assert(kTransitionGracePeriod >=
                kNetworkSettleDelay + base::Seconds(1));
  static_assert(kTransitionGracePeriod > base::Seconds(2));

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  net::test::ScopedMockNetworkChangeNotifier network_change_notifier_;
  FakeConnectionApi::Log api_log_;
  ConnectionManager manager_{
      base::BindRepeating(&ConnectionManagerSessionTest::CreateConnectionApi,
                          base::Unretained(this))};
  TestObserver observer_{manager_};
};

TEST_F(ConnectionManagerSessionTest, ConnectEstablishesTunnel) {
  manager_.Connect();
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(1, api_log_.connect_requests);

  api().ReportEstablished();
  EXPECT_EQ(State::kConnected, state());
  EXPECT_EQ((std::vector<State>{State::kConnecting, State::kConnected}),
            ObservedStates());
}

TEST_F(ConnectionManagerSessionTest, ConnectIgnoredWhenAlreadyUnderway) {
  manager_.Connect();
  manager_.Connect();
  EXPECT_EQ(1, api_log_.connect_requests);

  api().ReportEstablished();
  manager_.Connect();
  EXPECT_EQ(1, api_log_.connect_requests);
  EXPECT_EQ(State::kConnected, state());
}

TEST_F(ConnectionManagerSessionTest, DisconnectTakesTunnelDown) {
  ConnectAndEstablish();

  manager_.Disconnect();
  EXPECT_EQ(State::kDisconnecting, state());
  EXPECT_EQ(1, api_log_.disconnect_requests);

  // Progress information from the API changes nothing.
  api().ReportTearingDown();
  EXPECT_EQ(State::kDisconnecting, state());

  api().ReportTerminated();
  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ((std::vector<State>{State::kConnecting, State::kConnected,
                                State::kDisconnecting, State::kDisconnected}),
            ObservedStates());
}

TEST_F(ConnectionManagerSessionTest, DisconnectWithoutTunnelDoesNothing) {
  manager_.Disconnect();

  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ(0, api_log_.created);
  EXPECT_TRUE(ObservedStates().empty());
}

TEST_F(ConnectionManagerSessionTest, DisconnectAbortsAttempt) {
  manager_.Connect();

  manager_.Disconnect();
  EXPECT_EQ(State::kDisconnecting, state());
  EXPECT_EQ(1, api_log_.disconnect_requests);

  api().ReportTerminated();
  EXPECT_EQ(State::kDisconnected, state());
}

// The attempt can complete just before Disconnect() reaches the API. The result
// of the teardown is the one that counts.
TEST_F(ConnectionManagerSessionTest, EstablishedWhileDisconnectingIsIgnored) {
  manager_.Connect();
  manager_.Disconnect();

  api().ReportEstablished();
  EXPECT_EQ(State::kDisconnecting, state());

  api().ReportTerminated();
  EXPECT_EQ(State::kDisconnected, state());
}

// Once the user has disconnected, a failure is not an error to show and not a
// reason to retry.
TEST_F(ConnectionManagerSessionTest, FailureAfterDisconnectIsPlainDisconnect) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  manager_.Disconnect();

  FailAttempt();
  EXPECT_EQ(State::kDisconnected, state());

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ(1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, ConnectWaitsForTeardownToFinish) {
  ConnectAndEstablish();
  manager_.Disconnect();

  manager_.Connect();
  EXPECT_EQ(State::kDisconnecting, state());
  EXPECT_EQ(1, api_log_.connect_requests);

  // The API must not be called back from its own callback, so the attempt
  // starts from a task.
  api().ReportTerminated();
  EXPECT_EQ(State::kWaitingRetry, state());
  EXPECT_EQ(1, api_log_.connect_requests);

  WaitForAttemptAfterTeardown();
  EXPECT_EQ(2, api_log_.connect_requests);
}

// The teardown the user asked for is not a failed attempt: if the attempt
// that follows fails, the backoff starts from its first step.
TEST_F(ConnectionManagerSessionTest, ConnectAfterTeardownIsNotFailure) {
  manager_.SetAutoReconnectState(true);
  ConnectAndEstablish();
  manager_.Disconnect();
  manager_.Connect();
  api().ReportTerminated();
  WaitForAttemptAfterTeardown();
  ASSERT_EQ(2, api_log_.connect_requests);

  FailAttemptAndWaitForRetry(1);
  EXPECT_EQ(3, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, DisconnectCancelsConnectAfterTeardown) {
  ConnectAndEstablish();
  manager_.Disconnect();
  manager_.Connect();
  manager_.Disconnect();

  api().ReportTerminated();
  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ(1, api_log_.connect_requests);
}

// It is the user's own request, so unlike a retry it doesn't wait for the
// network to come back.
TEST_F(ConnectionManagerSessionTest, ConnectAfterTeardownIgnoresOutage) {
  ConnectAndEstablish();
  GoOffline();
  manager_.Disconnect();
  manager_.Connect();

  api().ReportTerminated();
  WaitForAttemptAfterTeardown();
  EXPECT_EQ(2, api_log_.connect_requests);
}

// A network change that arrives between the end of the teardown and the start
// of the attempt must not push the attempt back.
TEST_F(ConnectionManagerSessionTest, ConnectAfterTeardownIgnoresNetworkChange) {
  ConnectAndEstablish();
  manager_.Disconnect();
  manager_.Connect();
  api().ReportTerminated();

  DeliverNetworkChangeNow(net::NetworkChangeNotifier::CONNECTION_NONE);
  DeliverNetworkChangeNow(net::NetworkChangeNotifier::CONNECTION_WIFI);

  WaitForAttemptAfterTeardown();
  EXPECT_EQ(2, api_log_.connect_requests);
}

// Nor may an outage that begins in that gap leave the attempt parked.
TEST_F(ConnectionManagerSessionTest, ConnectAfterTeardownIgnoresNewOutage) {
  ConnectAndEstablish();
  manager_.Disconnect();
  manager_.Connect();
  api().ReportTerminated();

  network_change_notifier_.mock_network_change_notifier()->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  DeliverNetworkChangeNow(net::NetworkChangeNotifier::CONNECTION_NONE);

  WaitForAttemptAfterTeardown();
  EXPECT_EQ(2, api_log_.connect_requests);
}

// The pending attempt is the user's request, not a retry, so turning
// auto-reconnect off doesn't cancel it.
TEST_F(ConnectionManagerSessionTest, ConnectAfterTeardownIgnoresAutoReconnect) {
  manager_.SetAutoReconnectState(true);
  ConnectAndEstablish();
  manager_.Disconnect();
  manager_.Connect();
  api().ReportTerminated();

  manager_.SetAutoReconnectState(false);
  ASSERT_EQ(State::kWaitingRetry, state());

  WaitForAttemptAfterTeardown();
  EXPECT_EQ(2, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, FailedAttemptReportsConnectFailed) {
  manager_.Connect();

  FailAttempt();
  EXPECT_EQ(State::kConnectFailed, state());

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kConnectFailed, state());
  EXPECT_EQ(1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, TearingDownWhileConnectingIsIgnored) {
  manager_.Connect();

  api().ReportTearingDown();
  EXPECT_EQ(State::kConnecting, state());

  FailAttempt();
  EXPECT_EQ(State::kConnectFailed, state());
}

TEST_F(ConnectionManagerSessionTest, ResetConnectionStateClearsFailure) {
  manager_.Connect();
  FailAttempt();

  manager_.ResetConnectionState();
  EXPECT_EQ(State::kDisconnected, state());
}

TEST_F(ConnectionManagerSessionTest, ResetConnectionStateKeepsWorkingTunnel) {
  ConnectAndEstablish();

  manager_.ResetConnectionState();
  EXPECT_EQ(State::kConnected, state());
}

TEST_F(ConnectionManagerSessionTest, ConnectAfterFailureStartsNewAttempt) {
  manager_.Connect();
  FailAttempt();

  manager_.Connect();
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(2, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, DisconnectClearsFailure) {
  manager_.Connect();
  FailAttempt();

  manager_.Disconnect();
  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ(0, api_log_.disconnect_requests);
}

// A local problem is reported as such and never retried, even with
// auto-reconnect on.
TEST_F(ConnectionManagerSessionTest, InternalErrorIsNotRetried) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();

  api().ReportFailed(FailureReason::kInternalError);
  EXPECT_EQ(State::kInternalError, state());

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kInternalError, state());
  EXPECT_EQ(1, api_log_.connect_requests);

  manager_.ResetConnectionState();
  EXPECT_EQ(State::kDisconnected, state());
}

// A drop is not necessarily an error: without auto-reconnect the tunnel is
// simply gone.
TEST_F(ConnectionManagerSessionTest, DropWithoutAutoReconnectDisconnects) {
  ConnectAndEstablish();

  api().ReportTearingDown();
  EXPECT_EQ(State::kDisconnecting, state());

  api().ReportTerminated();
  EXPECT_EQ(State::kDisconnected, state());

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, TunnelFailureWithoutAutoReconnectFails) {
  ConnectAndEstablish();

  api().ReportFailed(FailureReason::kServerUnreachable);
  EXPECT_EQ(State::kConnectFailed, state());
}

// Callbacks that arrive when there is neither an attempt nor a tunnel answer
// nothing, and must not disturb the state the user sees.
TEST_F(ConnectionManagerSessionTest, CallbacksWithoutSessionAreIgnored) {
  manager_.Connect();
  FailAttempt();
  ASSERT_EQ(State::kConnectFailed, state());
  const auto updates = ObservedStates().size();

  api().ReportEstablished();
  api().ReportTearingDown();
  api().ReportTerminated();
  api().ReportFailed(FailureReason::kServerUnreachable);

  EXPECT_EQ(State::kConnectFailed, state());
  EXPECT_EQ(updates, ObservedStates().size());
}

TEST_F(ConnectionManagerSessionTest, SetAutoReconnectStateNotifies) {
  manager_.SetAutoReconnectState(true);

  EXPECT_TRUE(manager_.auto_reconnect_enabled());
  EXPECT_EQ((std::vector<bool>{true}),
            observer_.auto_reconnect_state_updates());
}

TEST_F(ConnectionManagerSessionTest, DroppedTunnelIsRestored) {
  manager_.SetAutoReconnectState(true);
  ConnectAndEstablish();

  api().ReportTerminated();
  EXPECT_EQ(State::kWaitingRetry, state());

  FastForwardBy(MaxRetryDelay(1));
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(2, api_log_.connect_requests);

  api().ReportEstablished();
  EXPECT_EQ(State::kConnected, state());
}

TEST_F(ConnectionManagerSessionTest, RetryDelayDoublesWithEveryFailure) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();

  for (int failures = 1; failures <= 4; ++failures) {
    SCOPED_TRACE(testing::Message() << "failures=" << failures);
    FailAttempt();
    EXPECT_EQ(State::kWaitingRetry, state());

    FastForwardBy(MinRetryDelay(failures) - kSessionTick);
    EXPECT_EQ(failures, api_log_.connect_requests);

    FastForwardBy(MaxRetryDelay(failures) - MinRetryDelay(failures) +
                  kSessionTick);
    EXPECT_EQ(failures + 1, api_log_.connect_requests);
    EXPECT_EQ(State::kConnecting, state());
  }
}

TEST_F(ConnectionManagerSessionTest, GivesUpAfterMaxReconnectAttempts) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();

  for (int failures = 1; failures <= kMaxReconnectAttempts; ++failures) {
    FailAttemptAndWaitForRetry(failures);
    ASSERT_EQ(failures + 1, api_log_.connect_requests);
  }

  FailAttempt();
  EXPECT_EQ(State::kConnectFailed, state());

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kConnectFailed, state());
  EXPECT_EQ(kMaxReconnectAttempts + 1, api_log_.connect_requests);
}

// A tunnel that comes up and drops again before it counts as stable keeps
// backing off, and running out of attempts is a failure even though every
// attempt succeeded.
TEST_F(ConnectionManagerSessionTest, FlappingTunnelBacksOffAndGivesUp) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();

  for (int drops = 1; drops <= kMaxReconnectAttempts; ++drops) {
    SCOPED_TRACE(testing::Message() << "drops=" << drops);
    api().ReportEstablished();
    FastForwardBy(kStableConnectionPeriod - kSessionTick);
    api().ReportTerminated();
    ASSERT_EQ(State::kWaitingRetry, state());

    FastForwardBy(MaxRetryDelay(drops));
    ASSERT_EQ(State::kConnecting, state());
  }

  api().ReportEstablished();
  FastForwardBy(kStableConnectionPeriod - kSessionTick);
  api().ReportTerminated();
  EXPECT_EQ(State::kConnectFailed, state());
  EXPECT_EQ(kMaxReconnectAttempts + 1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, StableConnectionResetsBackoff) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  for (int failures = 1; failures <= 3; ++failures) {
    FailAttemptAndWaitForRetry(failures);
  }
  ASSERT_EQ(4, api_log_.connect_requests);

  api().ReportEstablished();
  FastForwardBy(kStableConnectionPeriod);
  api().ReportTerminated();

  // Back to the first step of the backoff.
  FastForwardBy(MaxRetryDelay(1));
  EXPECT_EQ(5, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, ShortLivedConnectionKeepsBackoff) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  for (int failures = 1; failures <= 3; ++failures) {
    FailAttemptAndWaitForRetry(failures);
  }
  ASSERT_EQ(4, api_log_.connect_requests);

  api().ReportEstablished();
  FastForwardBy(kStableConnectionPeriod - kSessionTick);
  api().ReportTerminated();

  // The drop counts as the fourth failure in a row.
  FastForwardBy(MinRetryDelay(4) - kSessionTick);
  EXPECT_EQ(4, api_log_.connect_requests);

  FastForwardBy(MaxRetryDelay(4) - MinRetryDelay(4) + kSessionTick);
  EXPECT_EQ(5, api_log_.connect_requests);
}

// Nothing is in flight while waiting to retry, so a Connect() from the user
// must start an attempt and not just wait for the timer it cancelled.
TEST_F(ConnectionManagerSessionTest, ConnectWhileWaitingRetryStartsAttempt) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  FailAttemptAndWaitForRetry(1);
  FailAttempt();
  ASSERT_EQ(State::kWaitingRetry, state());
  ASSERT_EQ(2, api_log_.connect_requests);

  manager_.Connect();
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(3, api_log_.connect_requests);

  // It also begins a fresh retry sequence.
  FailAttemptAndWaitForRetry(1);
  EXPECT_EQ(4, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, DisconnectWhileWaitingRetryCancelsRetry) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  FailAttempt();
  ASSERT_EQ(State::kWaitingRetry, state());

  manager_.Disconnect();
  EXPECT_EQ(State::kDisconnected, state());
  // There is no tunnel to take down.
  EXPECT_EQ(0, api_log_.disconnect_requests);

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ(1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, DisablingAutoReconnectCancelsRetry) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  FailAttempt();
  ASSERT_EQ(State::kWaitingRetry, state());

  manager_.SetAutoReconnectState(false);
  EXPECT_EQ(State::kDisconnected, state());

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, NetworkChangeChecksTunnel) {
  ConnectAndEstablish();
  FastForwardBy(kTransitionGracePeriod);

  NotifyNetworkChanged();
  FastForwardBy(kNetworkSettleDelay - kSessionTick);
  EXPECT_EQ(0, api_log_.check_requests);

  FastForwardBy(kSessionTick);
  EXPECT_EQ(1, api_log_.check_requests);

  // A tunnel that still works changes nothing.
  api().ReportEstablished();
  EXPECT_EQ(State::kConnected, state());
  EXPECT_EQ((std::vector<State>{State::kConnecting, State::kConnected}),
            ObservedStates());
}

TEST_F(ConnectionManagerSessionTest, BurstOfNetworkChangesChecksTunnelOnce) {
  ConnectAndEstablish();
  FastForwardBy(kTransitionGracePeriod);

  NotifyNetworkChanged();
  FastForwardBy(kNetworkSettleDelay - kSessionTick);
  NotifyNetworkChanged();
  FastForwardBy(kNetworkSettleDelay - kSessionTick);
  EXPECT_EQ(0, api_log_.check_requests);

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(1, api_log_.check_requests);
}

// A change right after the tunnel came up is most likely the tunnel itself,
// but it is still checked once the grace period is over.
TEST_F(ConnectionManagerSessionTest, NetworkChangeAfterConnectIsCheckedLater) {
  ConnectAndEstablish();
  FastForwardBy(base::Seconds(1));

  NotifyNetworkChanged();
  FastForwardBy(kTransitionGracePeriod - base::Seconds(1) - kSessionTick);
  EXPECT_EQ(0, api_log_.check_requests);

  FastForwardBy(kSessionTick);
  EXPECT_EQ(1, api_log_.check_requests);
}

TEST_F(ConnectionManagerSessionTest, DeadTunnelFoundByCheckIsRestored) {
  manager_.SetAutoReconnectState(true);
  ConnectAndEstablish();
  FastForwardBy(kTransitionGracePeriod);
  NotifyNetworkChanged();
  FastForwardBy(kNetworkSettleDelay);
  ASSERT_EQ(1, api_log_.check_requests);

  api().ReportTerminated();
  EXPECT_EQ(State::kWaitingRetry, state());

  FastForwardBy(MaxRetryDelay(1));
  EXPECT_EQ(State::kConnecting, state());
}

TEST_F(ConnectionManagerSessionTest, DeadTunnelFoundByCheckDisconnects) {
  ConnectAndEstablish();
  FastForwardBy(kTransitionGracePeriod);
  NotifyNetworkChanged();
  FastForwardBy(kNetworkSettleDelay);
  ASSERT_EQ(1, api_log_.check_requests);

  api().ReportTerminated();
  EXPECT_EQ(State::kDisconnected, state());
}

TEST_F(ConnectionManagerSessionTest, NetworkChangeWithoutTunnelChecksNothing) {
  // Never connected: there isn't even an API to ask.
  NotifyNetworkChanged();
  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(0, api_log_.created);

  // After a failure, a check must not wipe the error the user sees.
  manager_.Connect();
  FailAttempt();
  NotifyNetworkChanged();
  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(0, api_log_.check_requests);
  EXPECT_EQ(State::kConnectFailed, state());
}

TEST_F(ConnectionManagerSessionTest, RetryWaitsForNetwork) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  FailAttempt();

  GoOffline();
  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kWaitingRetry, state());
  EXPECT_EQ(1, api_log_.connect_requests);

  GoOnline();
  FastForwardBy(kNetworkSettleDelay - kSessionTick);
  EXPECT_EQ(1, api_log_.connect_requests);

  FastForwardBy(kSessionTick);
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(2, api_log_.connect_requests);
}

// An attempt that fails during an outage is not retried until the network is
// back either.
TEST_F(ConnectionManagerSessionTest, FailureWhileOfflineWaitsForNetwork) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  GoOffline();

  FailAttempt();
  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kWaitingRetry, state());
  EXPECT_EQ(1, api_log_.connect_requests);

  GoOnline();
  FastForwardBy(kNetworkSettleDelay);
  EXPECT_EQ(2, api_log_.connect_requests);
}

// A failed attempt takes its tunnel down, which the notifier reports as a
// network change. Treating that as a new network would reset the backoff
// after every attempt, and the manager would retry forever.
TEST_F(ConnectionManagerSessionTest, OwnNetworkChurnDoesNotResetBackoff) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();

  for (int failures = 1; failures <= kMaxReconnectAttempts; ++failures) {
    // Both measured from the failure. The retry waits for the backoff, or for
    // the network to settle if that takes longer.
    const base::TimeDelta no_retry_before =
        MinRetryDelay(failures) - kSessionTick;
    const base::TimeDelta retry_by =
        std::max(kSessionTick + kNetworkSettleDelay, MaxRetryDelay(failures));

    FailAttempt();
    FastForwardBy(kSessionTick);
    NotifyNetworkChanged();

    FastForwardBy(no_retry_before - kSessionTick);
    ASSERT_EQ(failures, api_log_.connect_requests);

    FastForwardBy(retry_by - no_retry_before);
    ASSERT_EQ(failures + 1, api_log_.connect_requests);
  }

  FailAttempt();
  EXPECT_EQ(State::kConnectFailed, state());
  EXPECT_EQ(kMaxReconnectAttempts + 1, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, OwnNetworkChurnKeepsRetryDelay) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  for (int failures = 1; failures <= 4; ++failures) {
    FailAttemptAndWaitForRetry(failures);
  }
  FailAttempt();
  ASSERT_EQ(5, api_log_.connect_requests);

  FastForwardBy(base::Seconds(1));
  NotifyNetworkChanged();

  FastForwardBy(MinRetryDelay(5) - base::Seconds(1) - kSessionTick);
  EXPECT_EQ(5, api_log_.connect_requests);

  FastForwardBy(MaxRetryDelay(5) - MinRetryDelay(5) + kSessionTick);
  EXPECT_EQ(6, api_log_.connect_requests);
}

// Once the grace period is over, a network change is a new network: the retry
// is brought forward and the backoff starts over.
TEST_F(ConnectionManagerSessionTest, NetworkChangeBringsRetryForward) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  for (int failures = 1; failures <= 4; ++failures) {
    FailAttemptAndWaitForRetry(failures);
  }
  FailAttempt();
  ASSERT_EQ(5, api_log_.connect_requests);

  FastForwardBy(kTransitionGracePeriod + kSessionTick);
  NotifyNetworkChanged();

  FastForwardBy(kNetworkSettleDelay - kSessionTick);
  EXPECT_EQ(5, api_log_.connect_requests);

  FastForwardBy(kSessionTick);
  EXPECT_EQ(6, api_log_.connect_requests);

  FailAttemptAndWaitForRetry(1);
  EXPECT_EQ(7, api_log_.connect_requests);
}

// The tunnel going away doesn't take the network down, so the end of a real
// outage is a new network even inside the grace period.
TEST_F(ConnectionManagerSessionTest, EndOfOutageBringsRetryForward) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();
  for (int failures = 1; failures <= 4; ++failures) {
    FailAttemptAndWaitForRetry(failures);
  }
  FailAttempt();
  ASSERT_EQ(5, api_log_.connect_requests);

  FastForwardBy(base::Seconds(1));
  GoOffline();
  FastForwardBy(base::Seconds(1));
  GoOnline();

  FastForwardBy(kNetworkSettleDelay - kSessionTick);
  EXPECT_EQ(5, api_log_.connect_requests);

  FastForwardBy(kSessionTick);
  EXPECT_EQ(6, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, UnansweredConnectTimesOut) {
  manager_.Connect();

  FastForwardBy(kTransitionTimeout - kSessionTick);
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(0, api_log_.destroyed);

  // The stuck instance is destroyed, so it can't answer late.
  FastForwardBy(kSessionTick);
  EXPECT_EQ(State::kConnectFailed, state());
  EXPECT_EQ(1, api_log_.destroyed);

  // The next attempt gets a fresh one.
  manager_.Connect();
  EXPECT_EQ(2, api_log_.created);
  EXPECT_EQ(2, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, UnansweredConnectIsRetried) {
  manager_.SetAutoReconnectState(true);
  manager_.Connect();

  FastForwardBy(kTransitionTimeout);
  EXPECT_EQ(State::kWaitingRetry, state());
  EXPECT_EQ(1, api_log_.destroyed);

  FastForwardBy(MaxRetryDelay(1));
  EXPECT_EQ(State::kConnecting, state());
  EXPECT_EQ(2, api_log_.created);
  EXPECT_EQ(2, api_log_.connect_requests);
}

TEST_F(ConnectionManagerSessionTest, UnansweredDisconnectTimesOut) {
  ConnectAndEstablish();
  manager_.Disconnect();

  FastForwardBy(kTransitionTimeout - kSessionTick);
  EXPECT_EQ(State::kDisconnecting, state());

  FastForwardBy(kSessionTick);
  EXPECT_EQ(State::kDisconnected, state());
  EXPECT_EQ(1, api_log_.destroyed);
}

TEST_F(ConnectionManagerSessionTest, AnsweredConnectDoesNotTimeOut) {
  ConnectAndEstablish();

  FastForwardBy(kSessionLongTime);
  EXPECT_EQ(State::kConnected, state());
  EXPECT_EQ(0, api_log_.destroyed);
}

}  // namespace brave_vpn::v2
