/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_connection_api.h"

#include "base/check_is_test.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/base/network_change_notifier.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return std::make_unique<BraveVPNWireguardConnectionAPI>(url_loader_factory,
                                                          local_prefs, channel);
}

// TODO(spylogsster): Implement wireguard connection using Wireguard service.
BraveVPNWireguardConnectionAPI::BraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : local_prefs_(local_prefs),
      url_loader_factory_(url_loader_factory),
      region_data_manager_(url_loader_factory_, local_prefs_) {
  DCHECK(url_loader_factory_ && local_prefs_);
  // Safe to use Unretained here because |region_data_manager_| is owned
  // instance.
  region_data_manager_.set_selected_region_changed_callback(base::BindRepeating(
      &BraveVPNWireguardConnectionAPI::NotifySelectedRegionChanged,
      base::Unretained(this)));
  region_data_manager_.set_region_data_ready_callback(base::BindRepeating(
      &BraveVPNWireguardConnectionAPI::NotifyRegionDataReady,
      base::Unretained(this)));
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

BraveVPNWireguardConnectionAPI::~BraveVPNWireguardConnectionAPI() {}

std::string BraveVPNWireguardConnectionAPI::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNWireguardConnectionAPI::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  VLOG(1) << __func__ << " : " << type;
  CheckConnection();
}

BraveVpnAPIRequest* BraveVPNWireguardConnectionAPI::GetAPIRequest() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_) {
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  }

  return api_request_.get();
}

void BraveVPNWireguardConnectionAPI::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state) {
    return;
  }

  connection_state_ = state;
  for (auto& obs : observers_) {
    obs.OnConnectionStateChanged(connection_state_);
  }
}

mojom::ConnectionState BraveVPNWireguardConnectionAPI::GetConnectionState()
    const {
  return connection_state_;
}

void BraveVPNWireguardConnectionAPI::ResetConnectionState() {}

void BraveVPNWireguardConnectionAPI::RemoveVPNConnection() {}

void BraveVPNWireguardConnectionAPI::Connect() {}

void BraveVPNWireguardConnectionAPI::Disconnect() {}

void BraveVPNWireguardConnectionAPI::ToggleConnection() {}

void BraveVPNWireguardConnectionAPI::CheckConnection() {}

void BraveVPNWireguardConnectionAPI::ResetConnectionInfo() {}

std::string BraveVPNWireguardConnectionAPI::GetHostname() const {
  return std::string();
}

void BraveVPNWireguardConnectionAPI::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNWireguardConnectionAPI::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVPNWireguardConnectionAPI::SetConnectionState(
    mojom::ConnectionState state) {}

std::string BraveVPNWireguardConnectionAPI::GetLastConnectionError() const {
  return std::string();
}

BraveVPNRegionDataManager&
BraveVPNWireguardConnectionAPI::GetRegionDataManager() {
  return region_data_manager_;
}

void BraveVPNWireguardConnectionAPI::SetSelectedRegion(
    const std::string& name) {}

void BraveVPNWireguardConnectionAPI::NotifyRegionDataReady(bool ready) const {
  for (auto& obs : observers_) {
    obs.OnRegionDataReady(ready);
  }
}

void BraveVPNWireguardConnectionAPI::NotifySelectedRegionChanged(
    const std::string& name) const {
  for (auto& obs : observers_) {
    obs.OnSelectedRegionChanged(name);
  }
}

}  // namespace brave_vpn
