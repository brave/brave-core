/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/wireguard_connection_api_impl_win.h"

#include <memory>
#include <tuple>
#include <utility>

#include "brave/components/brave_vpn/common/win/utils.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/browser/brave_vpn/win/wireguard_utils_win.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

namespace {
// Timer to recheck the service launch after some time.
constexpr int kWireguardServiceRestartTimeoutSec = 5;
}  // namespace

using ConnectionState = mojom::ConnectionState;

WireguardConnectionAPIImplWin::WireguardConnectionAPIImplWin(
      BraveVPNConnectionManager* manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : WireguardConnectionAPIImplBase(manager, url_loader_factory) {}

WireguardConnectionAPIImplWin::~WireguardConnectionAPIImplWin() = default;

void WireguardConnectionAPIImplWin::Disconnect() {
  if (GetConnectionState() == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }
  VLOG(2) << __func__ << " : Start stopping the service";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);

  brave_vpn::wireguard::DisableBraveVpnWireguardService(
      base::BindOnce(&WireguardConnectionAPIImplWin::OnDisconnected,
                                weak_factory_.GetWeakPtr()));
}

void WireguardConnectionAPIImplWin::CheckConnection() {
  auto state = IsWindowsServiceRunning(
                   brave_vpn::GetBraveVpnWireguardTunnelServiceName())
                   ? ConnectionState::CONNECTED
                   : ConnectionState::DISCONNECTED;
  UpdateAndNotifyConnectionStateChange(state);
}

void WireguardConnectionAPIImplWin::PlatformConnectImpl(
    const wireguard::WireguardProfileCredentials& credentials) {
  auto vpn_server_hostname = GetHostname();
  brave_vpn::wireguard::EnableBraveVpnWireguardService(
      credentials.server_public_key, credentials.client_private_key,
      credentials.mapped_ip4_address, vpn_server_hostname,
      base::BindOnce(&WireguardConnectionAPIImplWin::OnWireguardServiceLaunched,
                     weak_factory_.GetWeakPtr()));
}

void WireguardConnectionAPIImplWin::OnServiceStopped(int mask) {
  // Postpone check because the service can be restarted by the system due to
  // configured failure actions.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&WireguardConnectionAPIImplWin::CheckConnection,
                     weak_factory_.GetWeakPtr()),
      base::Seconds(kWireguardServiceRestartTimeoutSec));
  ResetServiceWatcher();
}

void WireguardConnectionAPIImplWin::RunServiceWatcher() {
  if (service_watcher_ && service_watcher_->IsWatching()) {
    return;
  }
  service_watcher_.reset(new brave::ServiceWatcher());
  if (!service_watcher_->Subscribe(
          brave_vpn::GetBraveVpnWireguardTunnelServiceName(),
          SERVICE_NOTIFY_STOPPED,
          base::BindRepeating(
              &WireguardConnectionAPIImplWin::OnServiceStopped,
              weak_factory_.GetWeakPtr()))) {
    VLOG(1) << "Unable to set service watcher";
  }
}

void WireguardConnectionAPIImplWin::ResetServiceWatcher() {
  if (service_watcher_) {
    service_watcher_.reset();
  }
}

void WireguardConnectionAPIImplWin::OnWireguardServiceLaunched(
    bool success) {
  UpdateAndNotifyConnectionStateChange(
      success ? ConnectionState::CONNECTED : ConnectionState::CONNECT_FAILED);
}

void WireguardConnectionAPIImplWin::UpdateAndNotifyConnectionStateChange(
    mojom::ConnectionState state) {
  WireguardConnectionAPIImplBase::UpdateAndNotifyConnectionStateChange(state);
  if (state == ConnectionState::CONNECTED) {
    RunServiceWatcher();
    return;
  }
  ResetServiceWatcher();
}

}  // namespace brave_vpn
