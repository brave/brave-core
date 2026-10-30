/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_manager.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "mojo/public/cpp/bindings/clone_traits.h"

namespace brave_vpn::v2 {

FakeConnectionManager::FakeConnectionManager() = default;

FakeConnectionManager::~FakeConnectionManager() = default;

void FakeConnectionManager::Connect() {
  ++connect_calls_;
}

void FakeConnectionManager::Disconnect() {
  ++disconnect_calls_;
}

void FakeConnectionManager::ResetConnectionState() {
  ++reset_connection_state_calls_;
}

void FakeConnectionManager::GetAllRegions(GetAllRegionsCallback callback) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), mojo::Clone(regions_)));
}

void FakeConnectionManager::SetSelectedRegion(mojom::VpnRegionPtr region) {
  last_selected_region_ = std::move(region);
}

void FakeConnectionManager::SetAutoReconnectState(bool enable) {
  last_auto_reconnect_request_ = enable;
}

void FakeConnectionManager::SetSmartProxyRoutingState(bool enable) {
  last_smart_proxy_routing_request_ = enable;
}

void FakeConnectionManager::SetAllowLanTrafficState(bool enable) {
  last_allow_lan_traffic_request_ = enable;
}

}  // namespace brave_vpn::v2
