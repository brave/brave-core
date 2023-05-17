/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_connection_api.h"

#include "base/json/json_reader.h"
#include "brave/components/brave_vpn/browser/connection/common/win/utils.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/service_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

namespace {
constexpr char kCloudflareIPv4[] = "1.1.1.1";
}  // namespace

using ConnectionState = mojom::ConnectionState;

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return std::make_unique<BraveVPNWireguardConnectionAPI>(url_loader_factory,
                                                          local_prefs, channel);
}

BraveVPNWireguardConnectionAPI::BraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : BraveVPNOSConnectionAPI(url_loader_factory, local_prefs),
      local_prefs_(local_prefs) {
  DCHECK(local_prefs_);
}

BraveVPNWireguardConnectionAPI::~BraveVPNWireguardConnectionAPI() {}

std::string BraveVPNWireguardConnectionAPI::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNWireguardConnectionAPI::Connect() {
  if (GetConnectionState() == ConnectionState::CONNECTED) {
    Disconnect();
    return;
  }

  VLOG(2) << __func__ << " : start connecting!";
  SetLastConnectionError(std::string());
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);

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

void BraveVPNWireguardConnectionAPI::Disconnect() {
  if (GetConnectionState() == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }
  VLOG(2) << __func__ << " : Start stopping the service";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);

  brave_vpn::internal::DisableBraveVpnWireguardService(
      base::BindOnce(&BraveVPNWireguardConnectionAPI::OnDisconnected,
                     weak_factory_.GetWeakPtr()));
}

void BraveVPNWireguardConnectionAPI::OnDisconnected(bool success) {
  if (!success) {
    VLOG(1) << "Failed to stop wireguard tunnel service";
    return;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
}

void BraveVPNWireguardConnectionAPI::CheckConnection() {
  auto state = IsWindowsServiceRunning(
                   brave_vpn::GetBraveVpnWireguardTunnelServiceName())
                   ? ConnectionState::CONNECTED
                   : ConnectionState::DISCONNECTED;
  UpdateAndNotifyConnectionStateChange(state);
}

void BraveVPNWireguardConnectionAPI::FetchProfileCredentials() {
  if (!GetAPIRequest()) {
    return;
  }

  // Get profile credentials it to create OS VPN entry.
  VLOG(1) << __func__ << " : request profile credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());

  brave_vpn::internal::WireguardGenerateKeypair(base::BindOnce(
      &BraveVPNWireguardConnectionAPI::OnWireguardKeypairGenerated,
      weak_factory_.GetWeakPtr()));
}

void BraveVPNWireguardConnectionAPI::OnWireguardKeypairGenerated(
    brave_vpn::internal::WireguardKeyPair key_pair) {
  if (!key_pair.has_value()) {
    VLOG(1) << __func__ << " : failed to get keypair";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }
  const auto [public_key, private_key] = key_pair.value();

  GetAPIRequest()->GetWireguardProfileCredentials(
      base::BindOnce(&BraveVPNWireguardConnectionAPI::OnGetProfileCredentials,
                     base::Unretained(this), private_key),
      GetSubscriberCredential(local_prefs_), public_key, GetHostname());
}

void BraveVPNWireguardConnectionAPI::OnGetProfileCredentials(
    const std::string& client_private_key,
    const std::string& profile_credential,
    bool success) {
  if (!success) {
    VLOG(1) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value.has_value()) {
    auto* server_public_key =
        value->GetDict().FindStringByDottedPath("server-public-key");
    auto* mapped_pi4_address =
        value->GetDict().FindStringByDottedPath("mapped-ipv4-address");
    if (!server_public_key || !mapped_pi4_address) {
      VLOG(1) << __func__ << " : failed to get correct credentials";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }
    auto vpn_server_hostname = GetHostname();
    auto config = brave_vpn::internal::CreateWireguardConfig(
        client_private_key, *server_public_key, vpn_server_hostname,
        *mapped_pi4_address, kCloudflareIPv4);
    if (!config.has_value()) {
      VLOG(1) << __func__ << " : failed to get correct credentials";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }
    brave_vpn::internal::EnableBraveVpnWireguardService(
        config.value(),
        base::BindOnce(
            &BraveVPNWireguardConnectionAPI::OnWireguardServiceLaunched,
            weak_factory_.GetWeakPtr()));
  }
}

void BraveVPNWireguardConnectionAPI::OnWireguardServiceLaunched(bool success) {
  UpdateAndNotifyConnectionStateChange(
      success ? ConnectionState::CONNECTED : ConnectionState::CONNECT_FAILED);
}

void BraveVPNWireguardConnectionAPI::SetSelectedRegion(
    const std::string& name) {
  GetRegionDataManager().SetSelectedRegion(name);
}

}  // namespace brave_vpn
