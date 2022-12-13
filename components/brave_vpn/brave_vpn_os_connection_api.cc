/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/check_is_test.h"
#include "base/json/json_reader.h"
#include "base/power_monitor/power_monitor.h"
#include "brave/components/brave_vpn/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/brave_vpn/vpn_response_parser.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

BraveVPNOSConnectionAPI::BraveVPNOSConnectionAPI() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

BraveVPNOSConnectionAPI::~BraveVPNOSConnectionAPI() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void BraveVPNOSConnectionAPI::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNOSConnectionAPI::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVPNOSConnectionAPI::SetConnectionState(mojom::ConnectionState state) {
  UpdateAndNotifyConnectionStateChange(state);
}

bool BraveVPNOSConnectionAPI::IsInProgress() const {
  return connection_state_ == ConnectionState::DISCONNECTING ||
         connection_state_ == ConnectionState::CONNECTING;
}

void BraveVPNOSConnectionAPI::CreateVPNConnection() {
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (prevent_creation_) {
    CHECK_IS_TEST();
    return;
  }
  CreateVPNConnectionImpl(connection_info_);
}

void BraveVPNOSConnectionAPI::Connect() {
  if (IsInProgress()) {
    VLOG(2) << __func__ << ": Current state: " << connection_state_
            << " : prevent connecting while previous operation is in-progress";
    return;
  }

  // Ignore connect request while cancelling is in-progress.
  if (cancel_connecting_)
    return;

  // User can ask connect again when user want to change region.
  if (connection_state() == ConnectionState::CONNECTED) {
    // Disconnect first and then create again to setup for new region.
    // Set needs_connect to connect again after disconnected.
    needs_connect_ = true;
    Disconnect();
    return;
  }

  VLOG(2) << __func__ << " : start connecting!";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);

  if (GetIsSimulation() || connection_info_.IsValid()) {
    VLOG(2) << __func__
            << " : Create os vpn entry with cached connection_info.";
    CreateVPNConnectionImpl(connection_info_);
    return;
  }

  // If user doesn't select region explicitely, use default device region.
  std::string target_region_name = GetSelectedRegion();
  if (target_region_name.empty()) {
    target_region_name = GetDeviceRegion();
    VLOG(2) << __func__ << " : start connecting with valid default_region: "
            << target_region_name;
  }
  DCHECK(!target_region_name.empty());
  FetchHostnamesForRegion(target_region_name);
}

void BraveVPNOSConnectionAPI::Disconnect() {
  if (connection_state_ == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }

  if (connection_state_ == ConnectionState::DISCONNECTING) {
    VLOG(2) << __func__ << " : disconnecting in progress";
    return;
  }

  if (GetIsSimulation() || connection_state_ != ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << " : start disconnecting!";
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
    DisconnectImpl(target_vpn_entry_name_);
    return;
  }

  cancel_connecting_ = true;
  VLOG(2) << __func__ << " : Start cancelling connect request";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVPNOSConnectionAPI::ToggleConnection() {
  const bool can_disconnect =
      (connection_state_ == ConnectionState::CONNECTED ||
       connection_state_ == ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

void BraveVPNOSConnectionAPI::RemoveVPNConnection() {
  VLOG(2) << __func__;
  RemoveVPNConnectionImpl(target_vpn_entry_name_);
}

void BraveVPNOSConnectionAPI::CheckConnection() {
  CheckConnectionImpl(target_vpn_entry_name_);
}

void BraveVPNOSConnectionAPI::ResetConnectionInfo() {
  VLOG(2) << __func__;
  connection_info_.Reset();
}

std::string BraveVPNOSConnectionAPI::GetHostname() const {
  return hostname_ ? hostname_->hostname : "";
}

bool BraveVPNOSConnectionAPI::GetIsSimulation() const {
  return false;
}

void BraveVPNOSConnectionAPI::OnCreated() {
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  // It's time to ask connecting to os after vpn entry is created.
  ConnectImpl(target_vpn_entry_name_);
}

void BraveVPNOSConnectionAPI::OnCreateFailed() {
  VLOG(2) << __func__;

  // Clear connecting cancel request.
  if (cancel_connecting_)
    cancel_connecting_ = false;

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_NOT_ALLOWED);
}

void BraveVPNOSConnectionAPI::OnConnected() {
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    // As connect is done, we don't need more for cancelling.
    // Just start normal Disconenct() process.
    cancel_connecting_ = false;
    DisconnectImpl(target_vpn_entry_name_);
    return;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTED);
}

void BraveVPNOSConnectionAPI::OnIsConnecting() {
  VLOG(2) << __func__;

  if (!cancel_connecting_)
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);
}

void BraveVPNOSConnectionAPI::OnConnectFailed() {
  cancel_connecting_ = false;

  // Clear previously used connection info if failed.
  connection_info_.Reset();

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVPNOSConnectionAPI::OnDisconnected() {
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);

  if (needs_connect_) {
    needs_connect_ = false;
    Connect();
  }
}

void BraveVPNOSConnectionAPI::OnIsDisconnecting() {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVPNOSConnectionAPI::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // It's rare but sometimes Brave doesn't get vpn status update from OS.
  // Checking here will make vpn status update properly in that situation.
  VLOG(2) << __func__ << " : " << type;
  CheckConnection();
}

void BraveVPNOSConnectionAPI::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state)
    return;

  // Ignore disconnected state while connecting is in-progress.
  // Network status can be changed during the vpn connection because
  // establishing vpn connection could make system network offline temporarily.
  // Whenever we get network status change, we check vpn connection state and
  // it could give disconnected vpn connection during that situation.
  // So, don't notify this disconnected state change while connecting because
  // it's temporal state.
  if (connection_state_ == ConnectionState::CONNECTING &&
      state == ConnectionState::DISCONNECTED && !cancel_connecting_) {
    VLOG(2) << __func__ << ": Ignore disconnected state while connecting";
    return;
  }

#if BUILDFLAG(IS_WIN)
  // On Windows, we could get disconnected state after connect failed.
  // To make connect failed state as a last state, ignore disconnected state.
  if (connection_state_ == ConnectionState::CONNECT_FAILED &&
      state == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << ": Ignore disconnected state after connect failed";
    return;
  }
#endif  // BUILDFLAG(IS_WIN)
  VLOG(2) << __func__ << " : changing from " << connection_state_ << " to "
          << state;

  connection_state_ = state;
  for (auto& obs : observers_)
    obs.OnConnectionStateChanged(connection_state_);
}

BraveVpnAPIRequest* BraveVPNOSConnectionAPI::GetAPIRequest() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_) {
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  }

  return api_request_.get();
}

std::string BraveVPNOSConnectionAPI::GetDeviceRegion() const {
  return local_prefs_->GetString(prefs::kBraveVPNDeviceRegion);
}

std::string BraveVPNOSConnectionAPI::GetSelectedRegion() const {
  return local_prefs_->GetString(prefs::kBraveVPNSelectedRegion);
}

std::string BraveVPNOSConnectionAPI::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNOSConnectionAPI::FetchHostnamesForRegion(const std::string& name) {
  VLOG(2) << __func__;

  // Hostname will be replaced with latest one.
  hostname_.reset();

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  // Unretained is safe here becasue this class owns request helper.
  GetAPIRequest()->GetHostnamesForRegion(
      base::BindOnce(&BraveVPNOSConnectionAPI::OnFetchHostnames,
                     base::Unretained(this), name),
      name);
}

void BraveVPNOSConnectionAPI::OnFetchHostnames(const std::string& region,
                                               const std::string& hostnames,
                                               bool success) {
  VLOG(2) << __func__;
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, value->GetList());
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVPNOSConnectionAPI::ParseAndCacheHostnames(
    const std::string& region,
    const base::Value::List& hostnames_value) {
  std::vector<Hostname> hostnames = ParseHostnames(hostnames_value);

  if (hostnames.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  hostname_ = PickBestHostname(hostnames);
  if (hostname_->hostname.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : Picked " << hostname_->hostname << ", "
          << hostname_->display_name << ", " << hostname_->is_offline << ", "
          << hostname_->capacity_score;

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  // Get profile credentials it to create OS VPN entry.
  VLOG(2) << __func__ << " : request profile credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());

  GetAPIRequest()->GetProfileCredentials(
      base::BindOnce(&BraveVPNOSConnectionAPI::OnGetProfileCredentials,
                     base::Unretained(this)),
      GetSubscriberCredential(local_prefs_), hostname_->hostname);
}

void BraveVPNOSConnectionAPI::OnGetProfileCredentials(
    const std::string& profile_credential,
    bool success) {
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : received profile credential";

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value && value->is_dict()) {
    constexpr char kUsernameKey[] = "eap-username";
    constexpr char kPasswordKey[] = "eap-password";
    const auto& dict = value->GetDict();
    const std::string* username = dict.FindString(kUsernameKey);
    const std::string* password = dict.FindString(kPasswordKey);
    if (!username || !password) {
      VLOG(2) << __func__ << " : it's invalid profile credential";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }

    connection_info_.SetConnectionInfo(
        target_vpn_entry_name_, hostname_->hostname, *username, *password);
    // Let's create os vpn entry with |connection_info_|.
    CreateVPNConnection();
    return;
  }

  VLOG(2) << __func__ << " : it's invalid profile credential";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

}  // namespace brave_vpn
