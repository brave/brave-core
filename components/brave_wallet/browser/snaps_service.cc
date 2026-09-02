/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snaps_service.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/wallet_page_snap_bridge_controller.h"

namespace brave_wallet {

SnapsService::SnapsService(OpenWalletPageCallback open_wallet_page)
    : bridge_controller_(std::make_unique<WalletPageSnapBridgeController>(
          std::move(open_wallet_page))),
      snap_request_handler_(std::make_unique<SnapRequestHandlerImpl>()) {}

SnapsService::~SnapsService() = default;

void SnapsService::Bind(mojo::PendingReceiver<mojom::SnapsService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SnapsService::SetSnapBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  bridge_controller_->SetBridge(std::move(bridge));
}

void SnapsService::BindSnapRequestHandler(
    mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) {
  snap_request_handler_->Bind(std::move(receiver));
}

void SnapsService::InvokeSnap(const std::string& snap_id,
                              const std::string& method,
                              const std::string& params_json,
                              InvokeSnapCallback callback) {
  auto params = base::JSONReader::Read(params_json, base::JSON_PARSE_RFC);
  if (!params) {
    std::move(callback).Run(std::nullopt, "Invalid params JSON");
    return;
  }

  // Ensure the wallet page / hidden host is ready, then load the snap and
  // invoke the requested method.
  bridge_controller_->EnsureBridgeReady(base::BindOnce(
      [](base::WeakPtr<SnapsService> service, const std::string& snap_id,
         const std::string& method, base::Value params,
         const std::string& caller_origin, InvokeSnapCallback callback) {
        if (!service || !service->bridge_controller_->IsBound()) {
          std::move(callback).Run(std::nullopt, "SnapBridge disconnected");
          return;
        }
        service->bridge_controller_->LoadSnap(
            snap_id,
            base::BindOnce(&SnapsService::OnLoadSnapResult,
                           service->weak_ptr_factory_.GetWeakPtr(),
                           std::move(callback), snap_id, method,
                           std::move(params), caller_origin));
      },
      weak_ptr_factory_.GetWeakPtr(), snap_id, method, std::move(*params),
      /*caller_origin=*/"brave-wallet", std::move(callback)));
}

void SnapsService::OnLoadSnapResult(
    InvokeSnapCallback callback,
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    const std::string& caller_origin,
    bool success,
    const std::optional<std::string>& error) {
  if (!success) {
    std::move(callback).Run(std::nullopt,
                            error.value_or("Failed to load snap"));
    return;
  }
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(std::nullopt, "SnapBridge disconnected");
    return;
  }
  bridge_controller_->InvokeSnap(
      snap_id, method, std::move(params), caller_origin,
      base::BindOnce(&SnapsService::OnInvokeSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SnapsService::OnInvokeSnapResult(
    InvokeSnapCallback callback,
    std::optional<base::Value> result,
    const std::optional<std::string>& error) {
  std::optional<std::string> result_json;
  if (result) {
    result_json = base::WriteJson(*result);
  }
  std::move(callback).Run(result_json, error);
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

}  // namespace brave_wallet
