/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_bridge_controller.h"

#include <utility>

#include "base/functional/bind.h"

namespace brave_wallet {

WalletPageSnapBridgeController::WalletPageSnapBridgeController() = default;

WalletPageSnapBridgeController::~WalletPageSnapBridgeController() = default;

void WalletPageSnapBridgeController::SetBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  snap_bridge_.reset();
  snap_bridge_.Bind(std::move(bridge));
  snap_bridge_.set_disconnect_handler(
      base::BindOnce(&WalletPageSnapBridgeController::OnDisconnect,
                     weak_ptr_factory_.GetWeakPtr()));
}

bool WalletPageSnapBridgeController::IsBound() const {
  return snap_bridge_.is_bound();
}

void WalletPageSnapBridgeController::SetDisconnectCallback(
    DisconnectCallback cb) {
  disconnect_callback_ = std::move(cb);
}

void WalletPageSnapBridgeController::OnDisconnect() {
  snap_bridge_.reset();
  if (disconnect_callback_) {
    disconnect_callback_.Run();
  }
}

void WalletPageSnapBridgeController::LoadSnap(const std::string& snap_id,
                                              LoadSnapCallback cb) {
  snap_bridge_->LoadSnap(snap_id, std::move(cb));
}

void WalletPageSnapBridgeController::InvokeSnap(
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    const std::string& caller_origin,
    InvokeSnapCallback cb) {
  snap_bridge_->InvokeSnap(snap_id, method, std::move(params), caller_origin,
                           std::move(cb));
}

}  // namespace brave_wallet
