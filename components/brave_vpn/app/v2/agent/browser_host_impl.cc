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
  // follows through OnConnectionStateChanged().
  browser_endpoint_->OnConnectionStateChanged(
      connection_manager_->connection_state(),
      connection_manager_->error_message());
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

void BrowserHostImpl::OnConnectionStateChanged(
    mojom::VpnConnectionState state,
    std::optional<std::string> error_message) {
  browser_endpoint_->OnConnectionStateChanged(state, error_message);
}

}  // namespace brave_vpn::v2
