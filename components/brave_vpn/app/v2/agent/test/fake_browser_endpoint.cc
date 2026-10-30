/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_endpoint.h"

#include <utility>

#include "base/test/run_until.h"

namespace brave_vpn::v2 {

FakeBrowserEndpoint::FakeBrowserEndpoint() = default;

FakeBrowserEndpoint::~FakeBrowserEndpoint() = default;

void FakeBrowserEndpoint::OnConnectionStateChanged(
    mojom::VpnConnectionState state,
    const std::optional<std::string>& error_message) {
  connection_state_updates_.push_back(
      ConnectionStateUpdate{.state = state, .error_message = error_message});
}

void FakeBrowserEndpoint::OnSelectedRegionChanged(mojom::VpnRegionPtr region) {
  selected_region_updates_.push_back(std::move(region));
}

void FakeBrowserEndpoint::OnAutoReconnectStateChanged(bool enabled) {
  auto_reconnect_state_updates_.push_back(enabled);
}

void FakeBrowserEndpoint::OnSmartProxyRoutingStateChanged(bool enabled) {
  smart_proxy_routing_state_updates_.push_back(enabled);
}

void FakeBrowserEndpoint::OnAllowLanTrafficStateChanged(bool available,
                                                        bool enabled) {
  allow_lan_traffic_state_updates_.push_back(
      AllowLanTrafficStateUpdate{.available = available, .enabled = enabled});
}

void FakeBrowserEndpoint::OnLastHostnameChanged(const std::string& hostname) {
  last_hostname_updates_.push_back(hostname);
}

bool FakeBrowserEndpoint::WaitForConnectionStateUpdates(size_t count) {
  return base::test::RunUntil(
      [&] { return connection_state_updates_.size() >= count; });
}

}  // namespace brave_vpn::v2
