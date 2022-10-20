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
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/brave_vpn/vpn_response_parser.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

BraveVPNOSConnectionAPI::BraveVPNOSConnectionAPI() {
  base::PowerMonitor::AddPowerSuspendObserver(this);
  net::NetworkChangeNotifier::AddDNSObserver(this);
}

BraveVPNOSConnectionAPI::~BraveVPNOSConnectionAPI() {
  base::PowerMonitor::RemovePowerSuspendObserver(this);
  net::NetworkChangeNotifier::RemoveDNSObserver(this);
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

  DCHECK(!cancel_connecting_);

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

  if (!IsNetworkAvailable()) {
    VLOG(2) << __func__ << ": Network is not available, failed to connect";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  if (GetIsSimulation() || connection_info_.IsValid()) {
    VLOG(2) << __func__
            << " : direct connect as we already have valid connection info.";
    ConnectImpl(target_vpn_entry_name_);
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

void BraveVPNOSConnectionAPI::SetSkusCredential(const std::string& credential) {
  skus_credential_ = credential;
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
  UpdateAndNotifyConnectionStateChange(IsNetworkAvailable()
                                           ? ConnectionState::DISCONNECTED
                                           : ConnectionState::CONNECT_FAILED);

  if (needs_connect_) {
    needs_connect_ = false;

    if (connection_state_ == ConnectionState::DISCONNECTED)
      Connect();
  }
}

void BraveVPNOSConnectionAPI::OnIsDisconnecting() {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVPNOSConnectionAPI::OnSuspend() {
  // Set reconnection state in case if computer/laptop is going to sleep.
  // The disconnection event will be fired after waking up and we want to
  // restore the connection.
  if (connection_state_ == ConnectionState::CONNECTED) {
    Disconnect();
    reconnect_on_resume_ = true;
  }
  VLOG(2) << __func__
          << " Should reconnect when resume:" << reconnect_on_resume_;
}

void BraveVPNOSConnectionAPI::OnResume() {
#if BUILDFLAG(IS_MAC)
  OnDNSChanged();
#endif  // BUILDFLAG(IS_MAC)
}

void BraveVPNOSConnectionAPI::OnDNSChanged() {
  if (!IsNetworkAvailable() ||
      // This event is triggered before going to sleep while vpn is still
      // active. Vpn is presented as CONNECTION_UNKNOWN and so we have to skip
      // this to be notified only when default network active without VPN to
      // reconnect.
      net::NetworkChangeNotifier::GetConnectionType() ==
          net::NetworkChangeNotifier::CONNECTION_UNKNOWN)
    return;

  VLOG(2) << __func__ << " Should reconnect:" << reconnect_on_resume_;
  if (reconnect_on_resume_) {
    Connect();
    reconnect_on_resume_ = false;
  }
}

void BraveVPNOSConnectionAPI::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state)
    return;

#if BUILDFLAG(IS_WIN)
  // On Windows, we get disconnected status update twice.
  // When user connects to different region while connected,
  // we disconnect current connection and connect to newly selected
  // region. To do that we monitor |DISCONNECTED| state and start
  // connect when we get that state. But, Windows sends disconnected state
  // noti again. So, ignore second one.
  // On exception - we allow from connecting to disconnected in canceling
  // scenario.
  if (connection_state_ == ConnectionState::CONNECTING &&
      state == ConnectionState::DISCONNECTED && !cancel_connecting_) {
    VLOG(2) << __func__ << ": Ignore disconnected state while connecting";
    return;
  }

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

api_request_helper::APIRequestHelper*
BraveVPNOSConnectionAPI::GetAPIRequestHelper() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_helper_) {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            GetNetworkTrafficAnnotationTag(), url_loader_factory_);
  }

  return api_request_helper_.get();
}

std::string BraveVPNOSConnectionAPI::GetDeviceRegion() const {
  return local_prefs_->GetString(prefs::kBraveVPNDeviceRegion);
}

std::string BraveVPNOSConnectionAPI::GetSelectedRegion() const {
  return local_prefs_->GetString(prefs::kBraveVPNSelectedRegion);
}

std::string BraveVPNOSConnectionAPI::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEEnvironment);
}

void BraveVPNOSConnectionAPI::FetchHostnamesForRegion(const std::string& name) {
  VLOG(2) << __func__;
  // Hostname will be replaced with latest one.
  hostname_.reset();

  // Unretained is safe here becasue this class owns request helper.
  GetHostnamesForRegion(
      base::BindOnce(&BraveVPNOSConnectionAPI::OnFetchHostnames,
                     base::Unretained(this), name),
      name);
}

void BraveVPNOSConnectionAPI::GetHostnamesForRegion(ResponseCallback callback,
                                                    const std::string& region) {
  auto internal_callback =
      base::BindOnce(&BraveVPNOSConnectionAPI::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kHostnameForRegion);
  base::Value::Dict dict;
  dict.Set("region", region);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
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

  if (skus_credential_.empty()) {
    VLOG(2) << __func__ << " : skus_credential is empty";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  // Get subscriber credentials and then get EAP credentials with it to create
  // OS VPN entry.
  VLOG(2) << __func__ << " : request subscriber credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());
  GetSubscriberCredentialV12(
      base::BindOnce(&BraveVPNOSConnectionAPI::OnGetSubscriberCredentialV12,
                     base::Unretained(this)));
}

void BraveVPNOSConnectionAPI::GetSubscriberCredentialV12(
    ResponseCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVPNOSConnectionAPI::OnGetSubscriberCredential,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  const GURL base_url =
      GetURLWithPath(kVpnHost, kCreateSubscriberCredentialV12);
  base::Value::Dict dict;
  dict.Set("validation-method", "brave-premium");
  dict.Set("brave-vpn-premium-monthly-pass", skus_credential_);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback),
               {{"Brave-Payments-Environment",
                 GetBraveVPNPaymentsEnv(GetCurrentEnvironment())}});
}

void BraveVPNOSConnectionAPI::OnGetSubscriberCredential(
    ResponseCallback callback,
    api_request_helper::APIRequestResult api_request_result) {
  bool success = api_request_result.response_code() == 200;
  std::string error;
  std::string subscriber_credential =
      ParseSubscriberCredentialFromJson(api_request_result.body(), &error);
  if (!success) {
    subscriber_credential = error;
    VLOG(1) << __func__ << " Response from API was not HTTP 200 (Received "
            << api_request_result.response_code() << ")";
  }
  std::move(callback).Run(subscriber_credential, success);
}

void BraveVPNOSConnectionAPI::OnGetSubscriberCredentialV12(
    const std::string& subscriber_credential,
    bool success) {
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to get subscriber credential";
    if (subscriber_credential == kTokenNoLongerValid) {
      for (auto& obs : observers_)
        obs.OnGetInvalidToken();
    }
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : received subscriber credential";
  // TODO(bsclifton): consider storing `subscriber_credential` for
  // support ticket use-case (see `CreateSupportTicket`).
  GetProfileCredentials(
      base::BindOnce(&BraveVPNOSConnectionAPI::OnGetProfileCredentials,
                     base::Unretained(this)),
      subscriber_credential, hostname_->hostname);
}

void BraveVPNOSConnectionAPI::GetProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& hostname) {
  auto internal_callback =
      base::BindOnce(&BraveVPNOSConnectionAPI::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(hostname, kProfileCredential);
  base::Value::Dict dict;
  dict.Set("subscriber-credential", subscriber_credential);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
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

void BraveVPNOSConnectionAPI::OnGetResponse(
    ResponseCallback callback,
    api_request_helper::APIRequestResult result) {
  // NOTE: |api_request_helper_| uses JsonSanitizer to sanitize input made with
  // requests. |body| will be empty when the response from service is invalid
  // json.
  const bool success = result.response_code() == 200;
  std::move(callback).Run(result.body(), success);
}

void BraveVPNOSConnectionAPI::OAuthRequest(
    const GURL& url,
    const std::string& method,
    const std::string& post_data,
    URLRequestCallback callback,
    const base::flat_map<std::string, std::string>& headers) {
  if (!GetAPIRequestHelper())
    return;

  GetAPIRequestHelper()->Request(method, url, post_data, "application/json",
                                 true, std::move(callback), headers);
}

}  // namespace brave_vpn
