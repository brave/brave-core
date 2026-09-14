/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snaps_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_bridge_controller.h"

namespace brave_wallet {

SnapsService::SnapsService()
    : bridge_controller_(std::make_unique<WalletPageSnapBridgeController>()) {}

SnapsService::~SnapsService() = default;

void SnapsService::Bind(mojo::PendingReceiver<mojom::SnapsService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SnapsService::SetSnapBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  bridge_controller_->SetBridge(std::move(bridge));
}

void SnapsService::LoadSnap(const std::string& snap_id,
                            LoadSnapCallback callback) {
  // Snap execution lives in the already-open wallet page. Do not start a
  // page; fail if none is running.
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(false, "Wallet page is not running", std::nullopt);
    return;
  }
  bridge_controller_->LoadSnap(
      snap_id,
      base::BindOnce(&SnapsService::OnLoadSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapsService::OnLoadSnapResult(LoadSnapCallback callback,
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

void SnapsService::RequestInstallSnap(const std::string& snap_id,
                                      const std::string& version,
                                      RequestInstallSnapCallback callback) {
  // Minimal extraction: install simply stores a trivial bundle in memory.
  // A full implementation would download, verify, decompress and persist the
  // snap package.
  snap_bundles_[snap_id] =
      "// Minimal snap bundle for " + snap_id + "@" + version + "\n";
  std::move(callback).Run(true, std::nullopt);
}

void SnapsService::GetSnapBundle(const std::string& snap_id,
                                 GetSnapBundleCallback callback) {
  auto it = snap_bundles_.find(snap_id);
  if (it == snap_bundles_.end()) {
    std::move(callback).Run(std::nullopt, "Bundle not found");
    return;
  }
  std::move(callback).Run(it->second, std::nullopt);
}

void SnapsService::SetSnapBundleForTesting(const std::string& snap_id,
                                           const std::string& source_code) {
  snap_bundles_[snap_id] = source_code;
}

bool SnapsService::IsBridgeBoundForTesting() const {
  return bridge_controller_->IsBound();
}

}  // namespace brave_wallet
