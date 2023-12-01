/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/mac/brave_vpn_wireguard_connection_api_mac.h"

#include <memory>

#include "base/base64.h"
#include "base/notreached.h"

namespace brave_vpn {

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
  if (credentials.IsValid()) {
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
  NOTIMPLEMENTED();
}

void WireguardOSConnectionAPIMac::Disconnect() {
  if (GetConnectionState() == mojom::ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }
  VLOG(2) << __func__ << " : Start stopping the service";
  UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::DISCONNECTING);
  NOTIMPLEMENTED();
}

void WireguardOSConnectionAPIMac::CheckConnection() {
  NOTIMPLEMENTED();
}

}  // namespace brave_vpn
