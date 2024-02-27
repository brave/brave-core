/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/ikev2/system_vpn_connection_api_impl_base.h"

#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

SystemVPNConnectionAPIImplBase::~SystemVPNConnectionAPIImplBase() = default;

void SystemVPNConnectionAPIImplBase::SetSelectedRegion(
    const std::string& name) {
  // TODO(simonhong): Can remove this when UI block region changes while
  // operation is in-progress.
  // Don't allow region change while operation is in-progress.
  auto connection_state = GetConnectionState();
  if (connection_state == ConnectionState::DISCONNECTING ||
      connection_state == ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << ": Current state: " << connection_state
            << " : prevent changing selected region while previous operation "
               "is in-progress";
    // This is workaround to prevent UI changes seleted region.
    // Early return by notify again with current region name.
    manager_->NotifySelectedRegionChanged(
        manager_->GetRegionDataManager().GetSelectedRegion());
    return;
  }

  manager_->GetRegionDataManager().SetSelectedRegion(name);

  // As new selected region is used, |connection_info_| for previous selected
  // should be cleared.
  ResetConnectionInfo();
}

bool SystemVPNConnectionAPIImplBase::IsInProgress() const {
  return GetConnectionState() == ConnectionState::DISCONNECTING ||
         GetConnectionState() == ConnectionState::CONNECTING;
}

void SystemVPNConnectionAPIImplBase::CreateVPNConnection() {
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

void SystemVPNConnectionAPIImplBase::SetPreventCreationForTesting(bool value) {
  prevent_creation_ = value;
}

void SystemVPNConnectionAPIImplBase::Connect() {
  if (IsInProgress()) {
    VLOG(2) << __func__ << ": Current state: " << GetConnectionState()
            << " : prevent connecting while previous operation is in-progress";
    return;
  }

  // Ignore connect request while cancelling is in-progress.
  if (cancel_connecting_) {
    return;
  }

  // User can ask connect again when user want to change region.
  if (GetConnectionState() == ConnectionState::CONNECTED) {
    // Disconnect first and then create again to setup for new region.
    // Set needs_connect to connect again after disconnected.
    needs_connect_ = true;
    Disconnect();
    return;
  }

  VLOG(2) << __func__ << " : start connecting!";
  SetLastConnectionError(std::string());
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);

  if (connection_info_.IsValid()) {
    VLOG(2) << __func__
            << " : Create os vpn entry with cached connection_info.";
    CreateVPNConnectionImpl(connection_info_);
    return;
  }

  // If user doesn't select region explicitely, use default device region.
  std::string target_region_name =
      manager_->GetRegionDataManager().GetSelectedRegion();
  if (target_region_name.empty()) {
    target_region_name = manager_->GetRegionDataManager().GetDeviceRegion();
    VLOG(2) << __func__ << " : start connecting with valid default_region: "
            << target_region_name;
  }
  DCHECK(!target_region_name.empty());
  FetchHostnamesForRegion(target_region_name);
}

void SystemVPNConnectionAPIImplBase::Disconnect() {
  if (GetConnectionState() == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }

  if (GetConnectionState() == ConnectionState::DISCONNECTING) {
    VLOG(2) << __func__ << " : disconnecting in progress";
    return;
  }

  if (GetConnectionState() != ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << " : start disconnecting!";
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
    DisconnectImpl(manager_->target_vpn_entry_name());
    return;
  }

  cancel_connecting_ = true;
  VLOG(2) << __func__ << " : Start cancelling connect request";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);

  if (QuickCancelIfPossible()) {
    VLOG(2) << __func__ << " : Do quick cancel";
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
  }
}

void SystemVPNConnectionAPIImplBase::CheckConnection() {
  CheckConnectionImpl(manager_->target_vpn_entry_name());
}

void SystemVPNConnectionAPIImplBase::ResetConnectionInfo() {
  VLOG(2) << __func__;
  connection_info_.Reset();
}

void SystemVPNConnectionAPIImplBase::OnCreated() {
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  // It's time to ask connecting to os after vpn entry is created.
  ConnectImpl(manager_->target_vpn_entry_name());
}

void SystemVPNConnectionAPIImplBase::OnCreateFailed() {
  VLOG(2) << __func__;

  // Clear connecting cancel request.
  if (cancel_connecting_) {
    cancel_connecting_ = false;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_NOT_ALLOWED);
}

void SystemVPNConnectionAPIImplBase::OnConnected() {
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    // As connect is done, we don't need more for cancelling.
    // Just start normal Disconenct() process.
    cancel_connecting_ = false;
    DisconnectImpl(manager_->target_vpn_entry_name());
    return;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTED);
}

void SystemVPNConnectionAPIImplBase::OnIsConnecting() {
  VLOG(2) << __func__;

  if (!cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);
  }
}

void SystemVPNConnectionAPIImplBase::OnConnectFailed() {
  cancel_connecting_ = false;

  // Clear previously used connection info if failed.
  connection_info_.Reset();

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

bool SystemVPNConnectionAPIImplBase::MaybeReconnect() {
  VLOG(2) << __func__;

  if (!needs_connect_) {
    VLOG(2) << "Should be called only when reconnect expected";
    return false;
  }
  if (GetConnectionState() != ConnectionState::DISCONNECTED) {
    VLOG(2) << "For reconnection we expect DISCONNECTED status";
    return false;
  }
  if (IsPlatformNetworkAvailable()) {
    needs_connect_ = false;
    Connect();
    return true;
  }
  return false;
}

void SystemVPNConnectionAPIImplBase::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (needs_connect_ && MaybeReconnect()) {
    VLOG(2) << "Network is live, reconnecting";
    return;
  }
  ConnectionAPIImpl::OnNetworkChanged(type);
}

ConnectionAPIImpl::Type SystemVPNConnectionAPIImplBase::type() const {
  return Type::IKEV2;
}

void SystemVPNConnectionAPIImplBase::OnDisconnected() {
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
  // Sometimes disconnected event happens before network state restored,
  // we postpone reconnection in this cases.
  if (needs_connect_ && !MaybeReconnect()) {
    VLOG(2) << "Network is down, will be reconnected when connection restored";
  }
}

void SystemVPNConnectionAPIImplBase::OnIsDisconnecting() {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void SystemVPNConnectionAPIImplBase::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (GetConnectionState() == state) {
    return;
  }

  // Ignore disconnected state while connecting is in-progress.
  // Network status can be changed during the vpn connection because
  // establishing vpn connection could make system network offline temporarily.
  // Whenever we get network status change, we check vpn connection state and
  // it could give disconnected vpn connection during that situation.
  // So, don't notify this disconnected state change while connecting because
  // it's temporal state.
  if (GetConnectionState() == ConnectionState::CONNECTING &&
      state == ConnectionState::DISCONNECTED && !cancel_connecting_) {
    VLOG(2) << __func__ << ": Ignore disconnected state while connecting";
    return;
  }
#if BUILDFLAG(IS_WIN)
  // On Windows, we could get disconnected state after connect failed.
  // To make connect failed state as a last state, ignore disconnected state.
  if (GetConnectionState() == ConnectionState::CONNECT_FAILED &&
      state == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << ": Ignore disconnected state after connect failed";
    return;
  }
#endif  // BUILDFLAG(IS_WIN)
  VLOG(2) << __func__ << " : changing from " << GetConnectionState() << " to "
          << state;

  ConnectionAPIImpl::UpdateAndNotifyConnectionStateChange(state);
}

void SystemVPNConnectionAPIImplBase::FetchProfileCredentials() {
  GetAPIRequest()->GetProfileCredentials(
      base::BindOnce(&SystemVPNConnectionAPIImplBase::OnGetProfileCredentials,
                     base::Unretained(this)),
      GetSubscriberCredential(manager_->local_prefs()), GetHostname());
}

void SystemVPNConnectionAPIImplBase::OnGetProfileCredentials(
    const std::string& profile_credential,
    bool success) {
  DCHECK(!cancel_connecting_);

  if (!success) {
    VLOG(2) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  ResetAPIRequestInstance();

  VLOG(2) << __func__ << " : received profile credential";

  std::optional<base::Value> value = base::JSONReader::Read(profile_credential);
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

    connection_info_.SetConnectionInfo(manager_->target_vpn_entry_name(),
                                       GetHostname(), *username, *password);
    // Let's create os vpn entry with |connection_info_|.
    CreateVPNConnection();
    return;
  }

  VLOG(2) << __func__ << " : it's invalid profile credential";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

}  // namespace brave_vpn
