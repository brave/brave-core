/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

#include <optional>
#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_manager.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// Records every notification it receives, oldest first.
class TestObserver : public ConnectionManager::Observer {
 public:
  struct Update {
    mojom::VpnConnectionState state;
    std::optional<std::string> error_message;
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
    updates_.push_back(Update{.state = state, .error_message = error_message});
  }

  void StopObserving() { observation_.Reset(); }

  const std::vector<Update>& updates() const { return updates_; }

 private:
  std::vector<Update> updates_;
  base::ScopedObservation<ConnectionManager, ConnectionManager::Observer>
      observation_{this};
};

}  // namespace

// The state handling under test lives in ConnectionManager itself; the fake
// only exposes SetConnectionState(), so these tests exercise the real logic.
class ConnectionManagerTest : public testing::Test {
 protected:
  FakeConnectionManager manager_;
  TestObserver observer_{manager_};
};

TEST_F(ConnectionManagerTest, ReportsStateThroughGettersAndObserver) {
  EXPECT_EQ(mojom::VpnConnectionState::kDisconnected,
            manager_.connection_state());
  EXPECT_EQ(std::nullopt, manager_.error_message());

  manager_.SetConnectionState(mojom::VpnConnectionState::kConnectFailed,
                              "Server unreachable");

  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed,
            manager_.connection_state());
  EXPECT_EQ("Server unreachable", manager_.error_message());

  ASSERT_EQ(1u, observer_.updates().size());
  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed,
            observer_.updates()[0].state);
  EXPECT_EQ("Server unreachable", observer_.updates()[0].error_message);
}

TEST_F(ConnectionManagerTest, DoesNotNotifyWhenNothingChanges) {
  // Same as the initial state.
  manager_.SetConnectionState(mojom::VpnConnectionState::kDisconnected,
                              std::nullopt);
  EXPECT_TRUE(observer_.updates().empty());

  manager_.SetConnectionState(mojom::VpnConnectionState::kConnectFailed,
                              "Server unreachable");
  manager_.SetConnectionState(mojom::VpnConnectionState::kConnectFailed,
                              "Server unreachable");
  EXPECT_EQ(1u, observer_.updates().size());
}

TEST_F(ConnectionManagerTest, NotifiesWhenOnlyErrorMessageChanges) {
  manager_.SetConnectionState(mojom::VpnConnectionState::kConnectFailed,
                              "Server unreachable");

  // A different error at the same state.
  manager_.SetConnectionState(mojom::VpnConnectionState::kConnectFailed,
                              "Handshake timed out");

  // Clearing the error at the same state is a change too: observers still
  // showing the old message must hear about it.
  manager_.SetConnectionState(mojom::VpnConnectionState::kConnectFailed,
                              std::nullopt);

  ASSERT_EQ(3u, observer_.updates().size());
  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed,
            observer_.updates()[1].state);
  EXPECT_EQ("Handshake timed out", observer_.updates()[1].error_message);
  EXPECT_EQ(mojom::VpnConnectionState::kConnectFailed,
            observer_.updates()[2].state);
  EXPECT_EQ(std::nullopt, observer_.updates()[2].error_message);
  EXPECT_EQ(std::nullopt, manager_.error_message());
}

}  // namespace brave_vpn::v2
