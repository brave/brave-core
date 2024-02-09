/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_connection_api_win.h"

#include <memory>
#include <tuple>
#include <utility>

#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/browser/brave_vpn/win/wireguard_utils_win.h"

namespace brave_vpn {

namespace {
// Timer to recheck the service launch after some time.
constexpr int kWireguardServiceRestartTimeoutSec = 5;
}  // namespace

using ConnectionState = mojom::ConnectionState;

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    base::RepeatingCallback<bool()> service_installer) {
  return std::make_unique<BraveVPNWireguardConnectionAPIWin>(
      url_loader_factory, local_prefs, service_installer);
}

BraveVPNWireguardConnectionAPIWin::BraveVPNWireguardConnectionAPIWin(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    base::RepeatingCallback<bool()> service_installer)
    : BraveVPNWireguardConnectionAPIBase(url_loader_factory, local_prefs) {
  if (service_installer) {
    install_system_service_callback_ = std::move(service_installer);
  }
}

BraveVPNWireguardConnectionAPIWin::~BraveVPNWireguardConnectionAPIWin() {}

void BraveVPNWireguardConnectionAPIWin::Disconnect() {
  if (GetConnectionState() == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }
  VLOG(2) << __func__ << " : Start stopping the service";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);

  brave_vpn::wireguard::DisableBraveVpnWireguardService(
      base::BindOnce(&BraveVPNWireguardConnectionAPIWin::OnDisconnected,
                                weak_factory_.GetWeakPtr()));
}

void BraveVPNWireguardConnectionAPIWin::CheckConnection() {
  auto state = IsWindowsServiceRunning(
                   brave_vpn::GetBraveVpnWireguardTunnelServiceName())
                   ? ConnectionState::CONNECTED
                   : ConnectionState::DISCONNECTED;
  UpdateAndNotifyConnectionStateChange(state);
}

void BraveVPNWireguardConnectionAPIWin::PlatformConnectImpl(
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
      config.value(),
      base::BindOnce(
          &BraveVPNWireguardConnectionAPIWin::OnWireguardServiceLaunched,
          weak_factory_.GetWeakPtr()));
}

void BraveVPNWireguardConnectionAPIWin::OnServiceStopped(int mask) {
  // Postpone check because the service can be restarted by the system due to
  // configured failure actions.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNWireguardConnectionAPIWin::CheckConnection,
                     weak_factory_.GetWeakPtr()),
      base::Seconds(kWireguardServiceRestartTimeoutSec));
  ResetServiceWatcher();
}

void BraveVPNWireguardConnectionAPIWin::RunServiceWatcher() {
  if (service_watcher_ && service_watcher_->IsWatching()) {
    return;
  }
  service_watcher_.reset(new brave::ServiceWatcher());
  if (!service_watcher_->Subscribe(
          brave_vpn::GetBraveVpnWireguardTunnelServiceName(),
          SERVICE_NOTIFY_STOPPED,
          base::BindRepeating(
              &BraveVPNWireguardConnectionAPIWin::OnServiceStopped,
              weak_factory_.GetWeakPtr()))) {
    VLOG(1) << "Unable to set service watcher";
  }
}

void BraveVPNWireguardConnectionAPIWin::ResetServiceWatcher() {
  if (service_watcher_) {
    service_watcher_.reset();
  }
}

void BraveVPNWireguardConnectionAPIWin::OnWireguardServiceLaunched(
    bool success) {
  UpdateAndNotifyConnectionStateChange(
      success ? ConnectionState::CONNECTED : ConnectionState::CONNECT_FAILED);
}

void BraveVPNWireguardConnectionAPIWin::OnConnectionStateChanged(
    mojom::ConnectionState state) {
  BraveVPNWireguardConnectionAPIBase::OnConnectionStateChanged(state);
  if (state == ConnectionState::CONNECTED) {
    RunServiceWatcher();
    return;
  }
  ResetServiceWatcher();
}

}  // namespace brave_vpn
