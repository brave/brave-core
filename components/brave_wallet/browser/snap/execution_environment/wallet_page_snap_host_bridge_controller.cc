/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_host_bridge_controller.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace brave_wallet {

WalletPageSnapHostBridgeController::WalletPageSnapHostBridgeController() =
    default;

WalletPageSnapHostBridgeController::~WalletPageSnapHostBridgeController() =
    default;

void WalletPageSnapHostBridgeController::BindNewBridge(
    mojo::PendingRemote<mojom::SnapHostBridge> bridge) {
  snap_host_bridge_.reset();
  snap_host_bridge_.Bind(std::move(bridge));
  snap_host_bridge_.set_disconnect_handler(
      base::BindOnce(&WalletPageSnapHostBridgeController::OnDisconnect,
                     weak_ptr_factory_.GetWeakPtr()));
}

bool WalletPageSnapHostBridgeController::IsBound() const {
  return snap_host_bridge_.is_bound();
}

void WalletPageSnapHostBridgeController::OnDisconnect() {
  snap_host_bridge_.reset();
}

void WalletPageSnapHostBridgeController::LoadSnap(
    const std::string& snap_id,
    const std::string& source_code,
    LoadSnapCallback cb) {
  if (!IsBound()) {
    std::move(cb).Run(false, "Snap host bridge disconnected", std::nullopt);
    return;
  }
  snap_host_bridge_->LoadSnap(
      snap_id, source_code,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          std::move(cb), false,
          std::optional<std::string>("Snap host bridge disconnected"),
          std::nullopt));
}

void WalletPageSnapHostBridgeController::UnloadSnap(
    const std::string& snap_id) {
  if (!IsBound()) {
    return;
  }
  snap_host_bridge_->UnloadSnap(snap_id);
}

}  // namespace brave_wallet
