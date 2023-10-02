/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/mac/brave_vpn_wireguard_connection_api_mac.h"

#include <memory>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#import "brave/components/brave_vpn/browser/connection/wireguard/mac/wireguard_utils_mac.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {
const char kBraveVpnEntryName[] = "BraveVPNWireguard";
const char kBraveVpnEntryAccountName[] = "Brave Vpn Tunnel: ";
const char kBraveVpnEntryDescription[] = "wg-quick(8) config";
}  // namespace

std::unique_ptr<WireguardOSConnectionAPIMac>
CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return std::make_unique<WireguardOSConnectionAPIMac>(url_loader_factory,
                                                       local_prefs);
}

WireguardOSConnectionAPIMac::WireguardOSConnectionAPIMac(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs)
    : BraveVPNWireguardConnectionAPIBase(url_loader_factory, local_prefs) {}

WireguardOSConnectionAPIMac::~WireguardOSConnectionAPIMac() = default;

void WireguardOSConnectionAPIMac::PlatformConnectImpl(
    const wireguard::WireguardProfileCredentials& credentials) {
  VLOG(1) << __func__;
  if (!credentials.IsValid()) {
    VLOG(1) << __func__ << " : failed to get correct credentials";
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }
  auto vpn_server_hostname = GetHostname();
  auto config = brave_vpn::wireguard::CreateWireguardConfig(
      credentials.client_private_key, credentials.server_public_key,
      vpn_server_hostname, credentials.mapped_ip4_address);
  if (!config.has_value()) {
    VLOG(1) << __func__ << " : failed to get correct credentials";
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }
  UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::CONNECTING);

  FindTunnelProviderManager(
      kBraveVpnEntryName,
      base::BindOnce(&WireguardOSConnectionAPIMac::OnExistingProviderFound,
                     weak_factory_.GetWeakPtr(), config.value()));
}

void WireguardOSConnectionAPIMac::OnExistingProviderFound(
    const std::string& config,
    NETunnelProviderManager* tunnel_provider_manager) {
  bool success = tunnel_provider_manager != nullptr;
  VLOG(1) << __func__ << " found:" << success;
  VLOG(1) << __func__ << " config:" << config;

  if (success) {
    ActivateTunnelProvider(tunnel_provider_manager);
    return;
  }
  CreateNewTunnelProviderManager(
      config, kBraveVpnEntryName, kBraveVpnEntryAccountName,
      kBraveVpnEntryDescription,
      base::BindOnce(&WireguardOSConnectionAPIMac::OnTunnelProviderCreated,
                     weak_factory_.GetWeakPtr()));
}

void WireguardOSConnectionAPIMac::ActivateTunnelProvider(
    NETunnelProviderManager* tunnel_provider_manager) {
  LOG(ERROR) << __func__ << ":"
             << [tunnel_provider_manager localizedDescription];
  NSError* activation_error;
  [[tunnel_provider_manager connection]
      startVPNTunnelWithOptions:nil
                 andReturnError:&activation_error];
  bool success = activation_error == nil;
  if (!success) {
    LOG(ERROR) << "Tunnel activation error:" << NSErrorToUTF8(activation_error);
  }
  auto status = success ? mojom::ConnectionState::CONNECTED
                        : mojom::ConnectionState::CONNECT_FAILED;
  UpdateAndNotifyConnectionStateChange(status);
}

void WireguardOSConnectionAPIMac::OnTunnelProviderCreated(
    NETunnelProviderManager* tunnel_provider_manager) {
  bool success = tunnel_provider_manager != nullptr;
  VLOG(1) << __func__ << ":" << success;
  if (!success) {
    VLOG(1) << __func__ << " : failed to create tunnel provider";
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }
  ActivateTunnelProvider(tunnel_provider_manager);
}

void WireguardOSConnectionAPIMac::Disconnect() {
  VLOG(1) << __func__;
  if (GetConnectionState() == mojom::ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }
  VLOG(2) << __func__ << " : Start stopping the service";
  UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::DISCONNECTING);

  FindTunnelProviderManager(
      kBraveVpnEntryName,
      base::BindOnce(&WireguardOSConnectionAPIMac::OnDisconnectImpl,
                     weak_factory_.GetWeakPtr()));
}

void WireguardOSConnectionAPIMac::OnDisconnectImpl(
    NETunnelProviderManager* tunnel_provider_manager) {
  if (tunnel_provider_manager == nullptr) {
    VLOG(2) << __func__ << "Connection not found";
    UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::DISCONNECTED);
    return;
  }
  [[tunnel_provider_manager connection] stopVPNTunnel];
}

void WireguardOSConnectionAPIMac::OnCheckConnection(
    NETunnelProviderManager* tunnel_provider_manager) {
  if (tunnel_provider_manager == nullptr) {
    VLOG(2) << __func__ << "Connection not found";
    UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::DISCONNECTED);
    return;
  }
  auto current_status = [[tunnel_provider_manager connection] status];
  VLOG(2) << "CheckConnection: "
          << brave_vpn::NEVPNStatusToString(current_status);
  auto vpn_state = current_status == NEVPNStatusConnected
                       ? mojom::ConnectionState::CONNECTED
                       : mojom::ConnectionState::DISCONNECTED;
  UpdateAndNotifyConnectionStateChange(vpn_state);
}

void WireguardOSConnectionAPIMac::CheckConnection() {
  FindTunnelProviderManager(
      kBraveVpnEntryName,
      base::BindOnce(&WireguardOSConnectionAPIMac::OnCheckConnection,
                     weak_factory_.GetWeakPtr()));
}

}  // namespace brave_vpn
