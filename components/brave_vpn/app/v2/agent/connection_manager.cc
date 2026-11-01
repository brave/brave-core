/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/notimplemented.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_vpn::v2 {

ConnectionManager::ConnectionManager() = default;

ConnectionManager::~ConnectionManager() = default;

void ConnectionManager::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void ConnectionManager::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

bool ConnectionManager::HasObservers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !observers_.empty();
}

mojom::VpnConnectionState ConnectionManager::connection_state() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return connection_state_;
}

const std::optional<std::string>& ConnectionManager::error_message() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return error_message_;
}

const mojom::VpnRegion& ConnectionManager::current_region() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return *current_region_;
}

bool ConnectionManager::auto_reconnect_enabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auto_reconnect_enabled_;
}

bool ConnectionManager::smart_proxy_routing_enabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return smart_proxy_routing_enabled_;
}

bool ConnectionManager::allow_lan_traffic_available() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_lan_traffic_available_;
}

bool ConnectionManager::allow_lan_traffic_enabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_lan_traffic_enabled_;
}

const std::string& ConnectionManager::last_hostname() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return last_hostname_;
}

void ConnectionManager::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::ResetConnectionState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::GetAllRegions(GetAllRegionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), std::vector<mojom::VpnRegionPtr>()));
}

void ConnectionManager::SetSelectedRegion(mojom::VpnRegionPtr region) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::SetAutoReconnectState(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::SetSmartProxyRoutingState(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::SetAllowLanTrafficState(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/54641)
  // Implement once there are concrete connection API stubs.
  NOTIMPLEMENTED_LOG_ONCE();
}

void ConnectionManager::UpdateConnectionState(
    mojom::VpnConnectionState state,
    std::optional<std::string> error_message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == state && error_message_ == error_message) {
    return;
  }
  connection_state_ = state;
  error_message_ = std::move(error_message);

  observers_.Notify(&Observer::OnConnectionStateChanged, connection_state_,
                    error_message_);
}

void ConnectionManager::UpdateSelectedRegion(mojom::VpnRegionPtr region) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(region);
  if (current_region_->Equals(*region)) {
    return;
  }
  current_region_ = std::move(region);
  observers_.Notify(&Observer::OnSelectedRegionChanged, *current_region_);
}

void ConnectionManager::UpdateAutoReconnectState(bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto_reconnect_enabled_ == enabled) {
    return;
  }
  auto_reconnect_enabled_ = enabled;
  observers_.Notify(&Observer::OnAutoReconnectStateChanged, enabled);
}

void ConnectionManager::UpdateSmartProxyRoutingState(bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (smart_proxy_routing_enabled_ == enabled) {
    return;
  }
  smart_proxy_routing_enabled_ = enabled;
  observers_.Notify(&Observer::OnSmartProxyRoutingStateChanged, enabled);
}

void ConnectionManager::UpdateAllowLanTrafficState(bool available,
                                                   bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (allow_lan_traffic_available_ == available &&
      allow_lan_traffic_enabled_ == enabled) {
    return;
  }
  allow_lan_traffic_available_ = available;
  allow_lan_traffic_enabled_ = enabled;
  observers_.Notify(&Observer::OnAllowLanTrafficStateChanged, available,
                    enabled);
}

void ConnectionManager::UpdateLastHostname(std::string hostname) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (last_hostname_ == hostname) {
    return;
  }
  last_hostname_ = std::move(hostname);
  observers_.Notify(&Observer::OnLastHostnameChanged, last_hostname_);
}

}  // namespace brave_vpn::v2
