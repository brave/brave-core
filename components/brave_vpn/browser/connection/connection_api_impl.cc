/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/connection_api_impl.h"

#include <vector>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_helper.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

ConnectionAPIImpl::ConnectionAPIImpl(
    BraveVPNConnectionManager* manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : manager_(*manager), url_loader_factory_(url_loader_factory) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

ConnectionAPIImpl::~ConnectionAPIImpl() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void ConnectionAPIImpl::ToggleConnection() {
  const bool can_disconnect =
      (GetConnectionState() == mojom::ConnectionState::CONNECTED ||
       GetConnectionState() == mojom::ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

mojom::ConnectionState ConnectionAPIImpl::GetConnectionState() const {
  return connection_state_;
}

void ConnectionAPIImpl::ResetConnectionState() {
  // Don't use UpdateAndNotifyConnectionStateChange() to update connection state
  // and set state directly because we have a logic to ignore disconnected state
  // when connect failed.
  connection_state_ = mojom::ConnectionState::DISCONNECTED;
  manager_->NotifyConnectionStateChanged(connection_state_);
}

std::string ConnectionAPIImpl::GetLastConnectionError() const {
  return last_connection_error_;
}

void ConnectionAPIImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  VLOG(1) << __func__ << " : " << type;
  CheckConnection();
}

void ConnectionAPIImpl::SetLastConnectionError(const std::string& error) {
  VLOG(2) << __func__ << " : " << error;
  last_connection_error_ = error;
}

bool ConnectionAPIImpl::QuickCancelIfPossible() {
  if (!api_request_) {
    return false;
  }

  // We're waiting responce from vpn server.
  // Can do quick cancel in this situation by cancel that request.
  ResetAPIRequestInstance();
  return true;
}

void ConnectionAPIImpl::ResetAPIRequestInstance() {
  api_request_.reset();
}

BraveVpnAPIRequest* ConnectionAPIImpl::GetAPIRequest() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_) {
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  }

  return api_request_.get();
}

void ConnectionAPIImpl::FetchHostnamesForRegion(const std::string& name) {
  // Hostname will be replaced with latest one.
  hostname_.reset();

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  // Unretained is safe here becasue this class owns request helper.
  GetAPIRequest()->GetHostnamesForRegion(
      base::BindOnce(&ConnectionAPIImpl::OnFetchHostnames,
                     base::Unretained(this), name),
      name, manager_->GetRegionDataManager().GetRegionPrecisionForName(name));
}

void ConnectionAPIImpl::OnFetchHostnames(const std::string& region,
                                         const std::string& hostnames,
                                         bool success) {
  if (!success) {
    VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }

  ResetAPIRequestInstance();

  std::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, value->GetList());
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::CONNECT_FAILED);
}

void ConnectionAPIImpl::ParseAndCacheHostnames(
    const std::string& region,
    const base::Value::List& hostnames_value) {
  std::vector<Hostname> hostnames = ParseHostnames(hostnames_value);

  if (hostnames.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }

  hostname_ = PickBestHostname(hostnames);
  if (hostname_->hostname.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : Picked " << hostname_->hostname << ", "
          << hostname_->display_name << ", " << hostname_->is_offline << ", "
          << hostname_->capacity_score;

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  FetchProfileCredentials();
}

std::string ConnectionAPIImpl::GetHostname() const {
  return hostname_ ? hostname_->hostname : "";
}

void ConnectionAPIImpl::ResetHostname() {
  hostname_.reset();
}

void ConnectionAPIImpl::UpdateAndNotifyConnectionStateChange(
    mojom::ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state) {
    return;
  }

  connection_state_ = state;
  manager_->NotifyConnectionStateChanged(connection_state_);
}

void ConnectionAPIImpl::SetConnectionStateForTesting(
    mojom::ConnectionState state) {
  UpdateAndNotifyConnectionStateChange(state);
}

}  // namespace brave_vpn
