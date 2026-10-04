/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_controller.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_host_bridge_controller.h"

namespace brave_wallet {

SnapService::SnapService()
    : bridge_controller_(
          std::make_unique<WalletPageSnapHostBridgeController>()) {}

SnapService::~SnapService() = default;

void SnapService::Bind(mojo::PendingReceiver<mojom::SnapService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SnapService::BindSnapHostBridge(
    mojo::PendingRemote<mojom::SnapHostBridge> bridge) {
  bridge_controller_->BindNewBridge(std::move(bridge));
}

void SnapService::LoadSnap(const std::string& snap_id,
                           LoadSnapCallback callback) {
  // TODO(https://github.com/brave/brave-browser/issues/58686): this is
  // reachable from the wallet renderer with an arbitrary `snap_id`. It is
  // inert while `snap_bundles_` is only populated by tests, but needs a
  // permission check before real bundles are installable.

  // Snap execution is hosted by the already-open wallet page. Do not open a
  // page; fail if none is running.
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(false, "Wallet page is not running", std::nullopt);
    return;
  }

  auto it = snap_bundles_.find(snap_id);
  if (it == snap_bundles_.end()) {
    std::move(callback).Run(false, "Bundle not found", std::nullopt);
    return;
  }

  bridge_controller_->LoadSnap(
      snap_id, it->second,
      base::BindOnce(&SnapService::OnLoadSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapService::OnLoadSnapResult(LoadSnapCallback callback,
                                   bool success,
                                   const std::optional<std::string>& error,
                                   const std::optional<std::string>& result) {
  if (!success) {
    std::move(callback).Run(false, error.value_or("Failed to load snap"),
                            std::nullopt);
    return;
  }
  std::move(callback).Run(true, std::nullopt, result);
}

void SnapService::SetSnapBundleForTesting(const std::string& snap_id,
                                          const std::string& source_code) {
  snap_bundles_[snap_id] = source_code;
}

bool SnapService::IsBridgeBoundForTesting() const {
  return bridge_controller_->IsBound();
}

}  // namespace brave_wallet
