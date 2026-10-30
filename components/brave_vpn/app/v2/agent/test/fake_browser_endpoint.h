/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_ENDPOINT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_ENDPOINT_H_

#include <stddef.h>

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"

namespace brave_vpn::v2 {

// Stands in for the surface a browser exposes to the agent, and records what
// the agent sends to it. Owns the browser's end of that pipe.
class FakeBrowserEndpoint : public mojom::BrowserEndpoint {
 public:
  struct ConnectionStateUpdate {
    mojom::VpnConnectionState state = mojom::VpnConnectionState::kDisconnected;
    std::optional<std::string> error_message;
  };

  struct AllowLanTrafficStateUpdate {
    bool available = false;
    bool enabled = false;
  };

  FakeBrowserEndpoint();
  ~FakeBrowserEndpoint() override;

  FakeBrowserEndpoint(const FakeBrowserEndpoint&) = delete;
  FakeBrowserEndpoint& operator=(const FakeBrowserEndpoint&) = delete;

  // mojom::BrowserEndpoint:
  void OnConnectionStateChanged(
      mojom::VpnConnectionState state,
      const std::optional<std::string>& error_message) override;
  void OnSelectedRegionChanged(mojom::VpnRegionPtr region) override;
  void OnAutoReconnectStateChanged(bool enabled) override;
  void OnSmartProxyRoutingStateChanged(bool enabled) override;
  void OnAllowLanTrafficStateChanged(bool available, bool enabled) override;
  void OnLastHostnameChanged(const std::string& hostname) override;

  // Every update of each kind received, oldest first.
  const std::vector<ConnectionStateUpdate>& connection_state_updates() const {
    return connection_state_updates_;
  }
  const std::vector<mojom::VpnRegionPtr>& selected_region_updates() const {
    return selected_region_updates_;
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

  // Runs the current sequence until at least |count| connection state updates
  // have arrived. Returns false if they don't arrive before the action timeout.
  [[nodiscard]] bool WaitForConnectionStateUpdates(size_t count);

 private:
  std::vector<ConnectionStateUpdate> connection_state_updates_;
  std::vector<mojom::VpnRegionPtr> selected_region_updates_;
  std::vector<bool> auto_reconnect_state_updates_;
  std::vector<bool> smart_proxy_routing_state_updates_;
  std::vector<AllowLanTrafficStateUpdate> allow_lan_traffic_state_updates_;
  std::vector<std::string> last_hostname_updates_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_BROWSER_ENDPOINT_H_
