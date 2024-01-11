/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_connection_api.h"

#include <memory>
#include <tuple>
#include <utility>

#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"

namespace brave_vpn {

namespace {
// Timer to recheck the service launch after some time.
constexpr int kWireguardServiceRestartTimeoutSec = 5;
}  // namespace

using ConnectionState = mojom::ConnectionState;

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel,
    base::RepeatingCallback<bool()> service_installer) {
  return std::make_unique<BraveVPNWireguardConnectionAPI>(
      url_loader_factory, local_prefs, channel, service_installer);
}

BraveVPNWireguardConnectionAPI::BraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel,
    base::RepeatingCallback<bool()> service_installer)
    : BraveVPNWireguardConnectionAPIBase(url_loader_factory, local_prefs),
      channel_(channel) {
  if (service_installer) {
    install_system_service_callback_ = std::move(service_installer);
  }
}

BraveVPNWireguardConnectionAPI::~BraveVPNWireguardConnectionAPI() {}

void BraveVPNWireguardConnectionAPI::Disconnect() {
  if (GetConnectionState() == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }
  VLOG(2) << __func__ << " : Start stopping the service";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);

  brave_vpn::wireguard::DisableBraveVpnWireguardService(
      channel_, base::BindOnce(&BraveVPNWireguardConnectionAPI::OnDisconnected,
                               weak_factory_.GetWeakPtr()));
}

void BraveVPNWireguardConnectionAPI::CheckConnection() {
  auto state = IsWindowsServiceRunning(
                   brave_vpn::GetBraveVpnWireguardTunnelServiceName(channel_))
                   ? ConnectionState::CONNECTED
                   : ConnectionState::DISCONNECTED;
  UpdateAndNotifyConnectionStateChange(state);
}

void BraveVPNWireguardConnectionAPI::PlatformConnectImpl(
    const wireguard::WireguardProfileCredentials& credentials) {
  auto vpn_server_hostname = GetHostname();
  auto config = brave_vpn::wireguard::CreateWireguardConfig(
      credentials.client_private_key, credentials.server_public_key,
      vpn_server_hostname, credentials.mapped_ip4_address);
  if (!config.has_value()) {
    VLOG(1) << __func__ << " : failed to get correct credentials";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }
  brave_vpn::wireguard::EnableBraveVpnWireguardService(
      config.value(), channel_,
      base::BindOnce(
          &BraveVPNWireguardConnectionAPI::OnWireguardServiceLaunched,
          weak_factory_.GetWeakPtr()));
}

void BraveVPNWireguardConnectionAPI::OnServiceStopped(int mask) {
  // Postpone check because the service can be restarted by the system due to
  // configured failure actions.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNWireguardConnectionAPI::CheckConnection,
                     weak_factory_.GetWeakPtr()),
      base::Seconds(kWireguardServiceRestartTimeoutSec));
  ResetServiceWatcher();
}

void BraveVPNWireguardConnectionAPI::RunServiceWatcher() {
  if (service_watcher_ && service_watcher_->IsWatching()) {
    return;
  }
  service_watcher_.reset(new brave::ServiceWatcher());
  if (!service_watcher_->Subscribe(
          brave_vpn::GetBraveVpnWireguardTunnelServiceName(channel_),
          SERVICE_NOTIFY_STOPPED,
          base::BindRepeating(&BraveVPNWireguardConnectionAPI::OnServiceStopped,
                              weak_factory_.GetWeakPtr()))) {
    VLOG(1) << "Unable to set service watcher";
  }
}

void BraveVPNWireguardConnectionAPI::ResetServiceWatcher() {
  if (service_watcher_) {
    service_watcher_.reset();
  }
}

void BraveVPNWireguardConnectionAPI::OnWireguardServiceLaunched(bool success) {
  UpdateAndNotifyConnectionStateChange(
      success ? ConnectionState::CONNECTED : ConnectionState::CONNECT_FAILED);
}

void BraveVPNWireguardConnectionAPI::OnConnectionStateChanged(
    mojom::ConnectionState state) {
  BraveVPNWireguardConnectionAPIBase::OnConnectionStateChanged(state);
  if (state == ConnectionState::CONNECTED) {
    RunServiceWatcher();
    return;
  }
  ResetServiceWatcher();
}

}  // namespace brave_vpn
