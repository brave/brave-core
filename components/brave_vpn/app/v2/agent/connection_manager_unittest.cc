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

 private:
  std::vector<ConnectionStateUpdate> connection_state_updates_;
  std::vector<std::string> selected_region_name_updates_;
  std::vector<bool> auto_reconnect_state_updates_;
  std::vector<bool> smart_proxy_routing_state_updates_;
  std::vector<AllowLanTrafficStateUpdate> allow_lan_traffic_state_updates_;
  std::vector<std::string> last_hostname_updates_;
  base::ScopedObservation<ConnectionManager, ConnectionManager::Observer>
      observation_{this};
};

}  // namespace

// The state handling under test lives in ConnectionManager itself; the fake
// only exposes its Update*() methods, so these tests exercise the real logic.
class ConnectionManagerTest : public testing::Test {
 protected:
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

}  // namespace brave_vpn::v2
