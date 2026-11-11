/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_host_impl.h"

#include <utility>

#include "base/functional/bind.h"

namespace brave_vpn::v2 {

BrowserHostImpl::BrowserHostImpl(
    ConnectionManager& connection_manager,
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
    mojo::PendingReceiver<mojom::BrowserHost> receiver,
    base::OnceClosure disconnect_handler)
    : connection_manager_(connection_manager),
      disconnect_handler_(std::move(disconnect_handler)),
      browser_endpoint_(std::move(browser_endpoint)),
      receiver_(this, std::move(receiver)) {
  browser_endpoint_.set_disconnect_handler(base::BindOnce(
      &BrowserHostImpl::OnPipeDisconnected, base::Unretained(this)));
  receiver_.set_disconnect_handler(base::BindOnce(
      &BrowserHostImpl::OnPipeDisconnected, base::Unretained(this)));
  connection_manager_observation_.Observe(&connection_manager);

  // The browser learns the current state without asking; every later change
  // follows through the corresponding observer callbacks.
  OnConnectionStateChanged(connection_manager_->connection_state(),
                           connection_manager_->error_message());
  OnSelectedRegionChanged(connection_manager_->current_region());
  OnAutoReconnectStateChanged(connection_manager_->auto_reconnect_enabled());
  OnSmartProxyRoutingStateChanged(
      connection_manager_->smart_proxy_routing_enabled());
  OnAllowLanTrafficStateChanged(
      connection_manager_->allow_lan_traffic_available(),
      connection_manager_->allow_lan_traffic_enabled());
  OnLastHostnameChanged(connection_manager_->last_hostname());
}

BrowserHostImpl::~BrowserHostImpl() = default;

void BrowserHostImpl::OnPipeDisconnected() {
  // Whichever pipe closes first reports the session as gone. The other pipe's
  // handler is destroyed along with this object, so this normally runs once;
  // the guard covers a caller whose handler leaves this object alive.
  if (!disconnect_handler_) {
    return;
  }

  // May delete this: nothing below may touch members.
  std::move(disconnect_handler_).Run();
}

void BrowserHostImpl::Connect() {
  connection_manager_->Connect();
}

void BrowserHostImpl::Disconnect() {
  connection_manager_->Disconnect();
}

void BrowserHostImpl::ResetConnectionState() {
  connection_manager_->ResetConnectionState();
}

void BrowserHostImpl::GetAllRegions(GetAllRegionsCallback callback) {
  connection_manager_->GetAllRegions(std::move(callback));
}

void BrowserHostImpl::SetSelectedRegion(mojom::VpnRegionPtr region) {
  connection_manager_->SetSelectedRegion(std::move(region));
}

void BrowserHostImpl::SetAutoReconnectState(bool enable) {
  connection_manager_->SetAutoReconnectState(enable);
}

void BrowserHostImpl::SetSmartProxyRoutingState(bool enable) {
  connection_manager_->SetSmartProxyRoutingState(enable);
}

void BrowserHostImpl::SetAllowLanTrafficState(bool enable) {
  connection_manager_->SetAllowLanTrafficState(enable);
}

void BrowserHostImpl::OnConnectionStateChanged(
    mojom::VpnConnectionState state,
    std::optional<std::string> error_message) {
  browser_endpoint_->OnConnectionStateChanged(state, error_message);
}

void BrowserHostImpl::OnSelectedRegionChanged(const mojom::VpnRegion& region) {
  browser_endpoint_->OnSelectedRegionChanged(region.Clone());
}

void BrowserHostImpl::OnAutoReconnectStateChanged(bool enabled) {
  browser_endpoint_->OnAutoReconnectStateChanged(enabled);
}

void BrowserHostImpl::OnSmartProxyRoutingStateChanged(bool enabled) {
  browser_endpoint_->OnSmartProxyRoutingStateChanged(enabled);
}

void BrowserHostImpl::OnAllowLanTrafficStateChanged(bool available,
                                                    bool enabled) {
  browser_endpoint_->OnAllowLanTrafficStateChanged(available, enabled);
}

void BrowserHostImpl::OnLastHostnameChanged(const std::string& hostname) {
  browser_endpoint_->OnLastHostnameChanged(hostname);
}

}  // namespace brave_vpn::v2
