/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap_service.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_controller.h"

namespace brave_wallet {

SnapService::SnapService(
    KeyringService& keyring_service,
    std::unique_ptr<SnapHostBridgeController> bridge_controller)
    : keyring_service_(keyring_service),
      bridge_controller_(std::move(bridge_controller)) {
  CHECK(bridge_controller_);
  keyring_service_->AddObserver(keyring_observer_.BindNewPipeAndPassRemote());
}

SnapService::~SnapService() = default;

void SnapService::Bind(mojo::PendingReceiver<mojom::SnapService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SnapService::Shutdown() {
  bridge_controller_->Shutdown();
}

void SnapService::Locked() {
  bridge_controller_->Shutdown();
}

void SnapService::WalletReset() {
  bridge_controller_->Shutdown();
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

  // Fail fast rather than starting a host for a locked wallet. IsLockedSync()
  // is used instead of a cached Locked()/Unlocked() flag so this does not race
  // the async mojo observer delivery.
  if (keyring_service_->IsLockedSync()) {
    std::move(callback).Run(false, "Wallet is locked", std::nullopt);
    return;
  }

  auto it = snap_bundles_.find(snap_id);
  if (it == snap_bundles_.end()) {
    std::move(callback).Run(false, "Bundle not found", std::nullopt);
    return;
  }

  bridge_controller_->EnsureBridgeReady(base::BindOnce(
      &SnapService::OnBridgeReady, weak_ptr_factory_.GetWeakPtr(), snap_id,
      it->second, std::move(callback)));
}

void SnapService::OnBridgeReady(const std::string& snap_id,
                                const std::string& source_code,
                                LoadSnapCallback callback) {
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(false, bridge_controller_->GetUnavailableError(),
                            std::nullopt);
    return;
  }
  bridge_controller_->LoadSnap(
      snap_id, source_code,
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
