/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/values.h"
// keyring_service.h is included by the parent browser target that pulls in
// this source_set; the forward declaration in the header is sufficient here.

namespace brave_wallet {

SnapController::SnapController(KeyringService* keyring_service)
    : keyring_service_(keyring_service) {
  DCHECK(keyring_service_);
}

SnapController::~SnapController() = default;

void SnapController::BindSnapRequestHandler(
    mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void SnapController::SetSnapBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  snap_bridge_.reset();
  snap_bridge_.Bind(std::move(bridge));
}

void SnapController::InvokeSnap(const std::string& snap_id,
                                const std::string& method,
                                base::Value params,
                                SnapResultCallback callback) {
  if (!snap_bridge_.is_connected()) {
    std::move(callback).Run(std::nullopt, "SnapBridge is not connected");
    return;
  }

  // Load the snap first. The snap source is looked up from the registry; for
  // now we pass an empty source — snap_registry.cc will be wired up in a
  // subsequent step.
  const std::string snap_source;  // TODO(snap): look up from SnapRegistry

  snap_bridge_->LoadSnap(
      snap_id, snap_source,
      base::BindOnce(&SnapController::OnLoadSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), snap_id, method,
                     std::move(params), std::move(callback)));
}

void SnapController::HandleSnapRequest(const std::string& snap_id,
                                       const std::string& method,
                                       base::Value params,
                                       HandleSnapRequestCallback callback) {
  if (method == "snap_getBip44Entropy") {
    HandleGetBip44Entropy(snap_id, std::move(params), std::move(callback));
  } else if (method == "snap_getEntropy") {
    HandleGetEntropy(snap_id, std::move(params), std::move(callback));
  } else {
    DVLOG(1) << "SnapController: unhandled snap.request() method: " << method;
    std::move(callback).Run(std::nullopt,
                            "Unsupported snap method: " + method);
  }
}

// Private ------------------------------------------------------------------

void SnapController::HandleGetBip44Entropy(
    const std::string& snap_id,
    base::Value params,
    HandleSnapRequestCallback callback) {
  // TODO(snap): extract coin_type from params, call KeyringService to derive
  // the BIP-44 entropy for the snap's permitted coin type, and return it.
  // Placeholder until the full key derivation flow is implemented.
  std::move(callback).Run(std::nullopt,
                          "snap_getBip44Entropy not yet implemented");
}

void SnapController::HandleGetEntropy(const std::string& snap_id,
                                      base::Value params,
                                      HandleSnapRequestCallback callback) {
  // TODO(snap): derive snap-specific entropy from KeyringService using the
  // snap's ID as a salt, and return it.
  // Placeholder until the full key derivation flow is implemented.
  std::move(callback).Run(std::nullopt, "snap_getEntropy not yet implemented");
}

void SnapController::OnLoadSnapResult(
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    SnapResultCallback callback,
    bool success,
    const std::optional<std::string>& error) {
  if (!success) {
    std::move(callback).Run(
        std::nullopt, error.value_or("Failed to load snap: unknown error"));
    return;
  }

  snap_bridge_->InvokeSnap(
      snap_id, method, std::move(params),
      base::BindOnce(&SnapController::OnInvokeSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapController::OnInvokeSnapResult(
    SnapResultCallback callback,
    std::optional<base::Value> result,
    const std::optional<std::string>& error) {
  std::move(callback).Run(std::move(result), error);
}

}  // namespace brave_wallet
