/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_MANAGER_H_

#include <optional>
#include <utility>
#include <vector>

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

namespace brave_vpn::v2 {

// ConnectionManager for tests: its operations record what they receive, and
// tests drive state changes directly with the Update*() methods.
class FakeConnectionManager : public ConnectionManager {
 public:
  FakeConnectionManager();
  ~FakeConnectionManager() override;

  FakeConnectionManager(const FakeConnectionManager&) = delete;
  FakeConnectionManager& operator=(const FakeConnectionManager&) = delete;

  // ConnectionManager overrides:
  void Connect() override;
  void Disconnect() override;
  void ResetConnectionState() override;
  void GetAllRegions(GetAllRegionsCallback callback) override;
  void SetSelectedRegion(mojom::VpnRegionPtr region) override;
  void SetAutoReconnectState(bool enable) override;
  void SetSmartProxyRoutingState(bool enable) override;
  void SetAllowLanTrafficState(bool enable) override;

  // Lets tests check that observers registered and unregistered.
  using ConnectionManager::HasObservers;

  // Lets tests simulate the VPN changing state.
  using ConnectionManager::UpdateAllowLanTrafficState;
  using ConnectionManager::UpdateAutoReconnectState;
  using ConnectionManager::UpdateConnectionState;
  using ConnectionManager::UpdateLastHostname;
  using ConnectionManager::UpdateSelectedRegion;
  using ConnectionManager::UpdateSmartProxyRoutingState;

  int connect_calls() const { return connect_calls_; }
  int disconnect_calls() const { return disconnect_calls_; }
  int reset_connection_state_calls() const {
    return reset_connection_state_calls_;
  }
  void set_regions(std::vector<mojom::VpnRegionPtr> regions) {
    regions_ = std::move(regions);
  }
  const mojom::VpnRegionPtr& last_selected_region() const {
    return last_selected_region_;
  }
  std::optional<bool> last_auto_reconnect_request() const {
    return last_auto_reconnect_request_;
  }
  std::optional<bool> last_smart_proxy_routing_request() const {
    return last_smart_proxy_routing_request_;
  }
  std::optional<bool> last_allow_lan_traffic_request() const {
    return last_allow_lan_traffic_request_;
  }

 private:
  int connect_calls_ = 0;
  int disconnect_calls_ = 0;
  int reset_connection_state_calls_ = 0;
  std::vector<mojom::VpnRegionPtr> regions_;
  mojom::VpnRegionPtr last_selected_region_;
  std::optional<bool> last_auto_reconnect_request_;
  std::optional<bool> last_smart_proxy_routing_request_;
  std::optional<bool> last_allow_lan_traffic_request_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_MANAGER_H_
