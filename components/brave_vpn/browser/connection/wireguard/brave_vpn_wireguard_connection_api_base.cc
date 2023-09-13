/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"

#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

BraveVPNWireguardConnectionAPIBase::BraveVPNWireguardConnectionAPIBase(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs)
    : BraveVPNOSConnectionAPI(url_loader_factory, local_prefs) {
  AddObserver(this);
}

BraveVPNWireguardConnectionAPIBase::~BraveVPNWireguardConnectionAPIBase() {
  RemoveObserver(this);
}

void BraveVPNWireguardConnectionAPIBase::SetSelectedRegion(
    const std::string& name) {
  GetRegionDataManager().SetSelectedRegion(name);
  ResetConnectionInfo();
}

void BraveVPNWireguardConnectionAPIBase::RequestNewProfileCredentials(
    brave_vpn::wireguard::WireguardKeyPair key_pair) {
  if (!key_pair.has_value()) {
    VLOG(1) << __func__ << " : failed to get keypair";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    SetLastConnectionError("Failed to create keypair");
    return;
  }
  const auto [public_key, private_key] = key_pair.value();
  GetAPIRequest()->GetWireguardProfileCredentials(
      base::BindOnce(
          &BraveVPNWireguardConnectionAPIBase::OnGetProfileCredentials,
          base::Unretained(this), private_key),
      GetSubscriberCredential(local_prefs()), public_key, GetHostname());
}

void BraveVPNWireguardConnectionAPIBase::Connect() {
  VLOG(2) << __func__ << " : start connecting!";
  SetLastConnectionError(std::string());
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);

  // There's some fetched
  if (!GetHostname().empty()) {
    FetchProfileCredentials();
    return;
  }
  // If user doesn't select region explicitely, use default device region.
  std::string target_region_name = GetRegionDataManager().GetSelectedRegion();
  if (target_region_name.empty()) {
    target_region_name = GetRegionDataManager().GetDeviceRegion();
    VLOG(2) << __func__ << " : start connecting with valid default_region: "
            << target_region_name;
  }
  DCHECK(!target_region_name.empty());
  FetchHostnamesForRegion(target_region_name);
}

void BraveVPNWireguardConnectionAPIBase::OnGetProfileCredentials(
    const std::string& client_private_key,
    const std::string& profile_credentials,
    bool success) {
  if (!success) {
    VLOG(1) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    SetLastConnectionError("Failed to get profile credential");
    return;
  }
  auto parsed_credentials =
      wireguard::WireguardProfileCredentials::FromServerResponse(
          profile_credentials, client_private_key);
  if (!parsed_credentials.has_value()) {
    VLOG(1) << __func__ << " : failed to get correct credentials";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    SetLastConnectionError("Failed to get correct credentials");
    return;
  }
  auto serialized = parsed_credentials->ToString();
  if (serialized.has_value()) {
    local_prefs()->SetString(prefs::kBraveVPNWireguardProfileCredentials,
                             serialized.value());
  }
  PlatformConnectImpl(parsed_credentials.value());
}

void BraveVPNWireguardConnectionAPIBase::FetchProfileCredentials() {
  if (!GetAPIRequest()) {
    return;
  }
  auto existing_credentials =
      wireguard::WireguardProfileCredentials::FromString(
          local_prefs()->GetString(
              prefs::kBraveVPNWireguardProfileCredentials));
  if (!existing_credentials.has_value()) {
    RequestNewProfileCredentials(wireguard::GenerateNewX25519Keypair());
    return;
  }
  GetAPIRequest()->VerifyCredentials(
      base::BindOnce(&BraveVPNWireguardConnectionAPIBase::OnVerifyCredentials,
                     weak_factory_.GetWeakPtr()),
      GetHostname(), existing_credentials->client_id,
      GetSubscriberCredential(local_prefs()),
      existing_credentials->api_auth_token);
}

void BraveVPNWireguardConnectionAPIBase::ResetConnectionInfo() {
  VLOG(2) << __func__;
  ResetHostname();
  local_prefs()->SetString(prefs::kBraveVPNWireguardProfileCredentials,
                           std::string());
}

void BraveVPNWireguardConnectionAPIBase::OnVerifyCredentials(
    const std::string& result,
    bool success) {
  auto existing_credentials =
      wireguard::WireguardProfileCredentials::FromString(
          local_prefs()->GetString(
              prefs::kBraveVPNWireguardProfileCredentials));
  if (!success || !existing_credentials.has_value()) {
    VLOG(1) << __func__ << " : credentials verification failed ( " << result
            << " ), request new";
    RequestNewProfileCredentials(wireguard::GenerateNewX25519Keypair());
    return;
  }
  PlatformConnectImpl(existing_credentials.value());
}

void BraveVPNWireguardConnectionAPIBase::OnConnectionStateChanged(
    mojom::ConnectionState state) {
  if (state == ConnectionState::CONNECT_FAILED) {
    ResetConnectionInfo();
  }
}

void BraveVPNWireguardConnectionAPIBase::OnDisconnected(bool success) {
  if (!success) {
    VLOG(1) << "Failed to stop wireguard tunnel service";
    SetLastConnectionError("Failed to stop wireguard tunnel service");
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTED);
    return;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
}

}  // namespace brave_vpn
