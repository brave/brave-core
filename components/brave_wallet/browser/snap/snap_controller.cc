/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_bridge_controller.h"
#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace brave_wallet {

SnapController::SnapController(SnapDataProvider& data_provider,
                               SnapPermissionController& permission_controller,
                               SnapBridgeController& bridge_controller)
    : data_provider_(data_provider),
      permission_controller_(permission_controller),
      bridge_controller_(bridge_controller) {
  bridge_controller_->SetDisconnectCallback(base::BindRepeating(
      &SnapController::OnBridgeDisconnected, weak_ptr_factory_.GetWeakPtr()));
}

SnapController::~SnapController() = default;

void SnapController::SetInstallSnapDelegate(InstallSnapDelegate delegate) {
  install_snap_delegate_ = std::move(delegate);
}

void SnapController::SetRequestConnectionDelegate(
    RequestConnectionDelegate delegate) {
  request_connection_delegate_ = std::move(delegate);
}

SnapController::RequestSnapsState::RequestSnapsState() = default;
SnapController::RequestSnapsState::~RequestSnapsState() = default;

void SnapController::OnBridgeDisconnected() {
  for (size_t i = 0; i < pending_callbacks_.size(); ++i) {
    FailPendingCallback(i, "SnapBridge disconnected");
  }
}

void SnapController::FailPendingCallback(size_t index,
                                         const std::string& error) {
  if (index >= pending_callbacks_.size() || !pending_callbacks_[index]) {
    return;
  }
  std::move(pending_callbacks_[index]).Run(std::nullopt, error);
}

// ---------------------------------------------------------------------------
// InvokeSnap
// ---------------------------------------------------------------------------

void SnapController::InvokeSnap(const std::string& snap_id,
                                const std::string& method,
                                base::Value params,
                                std::optional<url::Origin> caller_origin,
                                SnapResultCallback callback) {
  if (caller_origin.has_value()) {
    const url::Origin& origin = *caller_origin;
    if (origin.opaque() || (origin.scheme() != url::kHttpScheme &&
                            origin.scheme() != url::kHttpsScheme)) {
      std::move(callback).Run(std::nullopt, "requires a secure web origin");
      return;
    }
    // A dApp invoking a snap it isn't connected to yet must obtain the user's
    // approval before the connection is granted.
    if (permission_controller_->IsOriginAllowedByManifest(origin, snap_id) &&
        !permission_controller_->IsSnapConnected(origin, snap_id)) {
      if (request_connection_delegate_) {
        request_connection_delegate_.Run(
            origin, snap_id,
            base::BindOnce(&SnapController::OnInvokeConnectionResult,
                           weak_ptr_factory_.GetWeakPtr(), snap_id, method,
                           std::move(params), caller_origin,
                           std::move(callback)));
        return;
      }
      // No approval delegate wired (e.g. in unit tests): auto-grant.
      permission_controller_->GrantSnapConnection(origin, snap_id);
    }
  }

  ContinueInvokeSnap(snap_id, method, std::move(params),
                     std::move(caller_origin), std::move(callback));
}

void SnapController::OnInvokeConnectionResult(
    std::string snap_id,
    std::string method,
    base::Value params,
    std::optional<url::Origin> caller_origin,
    SnapResultCallback callback,
    bool approved) {
  if (!approved) {
    std::move(callback).Run(std::nullopt, "user_rejected");
    return;
  }
  if (caller_origin.has_value()) {
    permission_controller_->GrantSnapConnection(*caller_origin, snap_id);
  }
  ContinueInvokeSnap(snap_id, method, std::move(params),
                     std::move(caller_origin), std::move(callback));
}

void SnapController::ContinueInvokeSnap(
    std::string snap_id,
    std::string method,
    base::Value params,
    std::optional<url::Origin> caller_origin,
    SnapResultCallback callback) {
  if (!data_provider_->IsInstalled(snap_id)) {
    std::move(callback).Run(std::nullopt, "Unknown snap: " + snap_id);
    return;
  }
  if (!data_provider_->IsSnapEnabled(snap_id)) {
    std::move(callback).Run(std::nullopt, "Snap is disabled: " + snap_id);
    return;
  }

  if (caller_origin.has_value()) {
    const url::Origin& origin = *caller_origin;
    if (!permission_controller_->IsSnapConnected(origin, snap_id)) {
      std::move(callback).Run(std::nullopt,
                              "Snap is not connected to this origin");
      return;
    }
    if (!permission_controller_->IsOriginAllowedByManifest(origin, snap_id)) {
      std::move(callback).Run(std::nullopt,
                              "Origin not permitted by snap manifest");
      return;
    }
  }

  const std::string origin_str =
      caller_origin ? caller_origin->Serialize() : "brave-wallet";

  const size_t cb_index = pending_callbacks_.size();
  pending_callbacks_.push_back(std::move(callback));

  bridge_controller_->EnsureBridgeReady(base::BindOnce(
      &SnapController::DispatchInvoke, weak_ptr_factory_.GetWeakPtr(), cb_index,
      snap_id, method, std::move(params), origin_str));
}

void SnapController::DispatchInvoke(size_t cb_index,
                                    std::string snap_id,
                                    std::string method,
                                    base::Value params,
                                    std::string origin_str) {
  if (!bridge_controller_->IsBound()) {
    FailPendingCallback(cb_index, "no_bridge");
    return;
  }
  bridge_controller_->LoadSnap(
      snap_id, base::BindOnce(&SnapController::OnLoadSnapResult,
                              weak_ptr_factory_.GetWeakPtr(), cb_index, snap_id,
                              method, std::move(params), origin_str));
}

// ---------------------------------------------------------------------------
// RequestSnaps
// ---------------------------------------------------------------------------

void SnapController::RequestSnaps(const url::Origin& origin,
                                  const base::DictValue& snaps_dict,
                                  RequestSnapsCallback callback) {
  if (origin.opaque() || (origin.scheme() != url::kHttpScheme &&
                          origin.scheme() != url::kHttpsScheme)) {
    std::move(callback).Run(std::nullopt,
                            "wallet_requestSnaps requires a secure web origin");
    return;
  }

  struct RequestItem {
    std::string snap_id;
    std::string version;
    bool installed = false;
  };
  std::vector<RequestItem> items;
  for (const auto [snap_id, opts] : snaps_dict) {
    std::string version = "latest";
    if (opts.is_dict()) {
      if (const std::string* v = opts.GetDict().FindString("version")) {
        version = *v;
      }
    }
    items.push_back(
        {snap_id, std::move(version), data_provider_->IsInstalled(snap_id)});
  }

  if (items.empty()) {
    std::move(callback).Run(base::DictValue(), std::nullopt);
    return;
  }

  auto state = std::make_shared<RequestSnapsState>();
  state->remaining = items.size();
  state->callback = std::move(callback);
  state->origin = origin;

  for (const auto& item : items) {
    if (item.installed && !data_provider_->IsSnapEnabled(item.snap_id)) {
      OnSnapConnectionResolved(state, item.snap_id, /*approved=*/false);
      continue;
    }
    if (!item.installed) {
      // Missing snaps are installed via the install-approval flow, which is the
      // user's consent to the connection.
      install_snap_delegate_.Run(
          item.snap_id, item.version,
          base::BindOnce(&SnapController::OnSingleSnapInstalled,
                         weak_ptr_factory_.GetWeakPtr(), state, item.snap_id));
    } else if (permission_controller_->IsSnapConnected(origin, item.snap_id)) {
      // Already connected — no prompt needed.
      OnSnapConnectionResolved(state, item.snap_id, /*approved=*/true);
    } else if (request_connection_delegate_) {
      // Installed but not yet connected to this origin — ask the user.
      request_connection_delegate_.Run(
          origin, item.snap_id,
          base::BindOnce(&SnapController::OnSnapConnectionResolved,
                         weak_ptr_factory_.GetWeakPtr(), state, item.snap_id));
    } else {
      // No approval delegate wired (e.g. in unit tests): auto-grant.
      OnSnapConnectionResolved(state, item.snap_id, /*approved=*/true);
    }
  }
}

void SnapController::OnSingleSnapInstalled(
    std::shared_ptr<RequestSnapsState> state,
    const std::string& snap_id,
    base::expected<void, std::string> result) {
  if (result.has_value()) {
    permission_controller_->GrantSnapConnection(state->origin, snap_id);
    state->result_dict.Set(snap_id, base::DictValue());
  }
  if (--state->remaining == 0) {
    std::move(state->callback).Run(std::move(state->result_dict), std::nullopt);
  }
}

void SnapController::OnSnapConnectionResolved(
    std::shared_ptr<RequestSnapsState> state,
    const std::string& snap_id,
    bool approved) {
  if (approved) {
    permission_controller_->GrantSnapConnection(state->origin, snap_id);
    state->result_dict.Set(snap_id, base::DictValue());
  }
  if (--state->remaining == 0) {
    std::move(state->callback).Run(std::move(state->result_dict), std::nullopt);
  }
}

// ---------------------------------------------------------------------------
// Homepage / user input
// ---------------------------------------------------------------------------

void SnapController::GetSnapHomePage(const std::string& snap_id,
                                     SnapHomePageCallback callback) {
  if (!data_provider_->IsSnapEnabled(snap_id)) {
    std::move(callback).Run(std::nullopt, std::nullopt, "Snap is disabled");
    return;
  }
  bridge_controller_->EnsureBridgeReady(base::BindOnce(
      &SnapController::DispatchGetSnapHomePage, weak_ptr_factory_.GetWeakPtr(),
      snap_id, std::move(callback)));
}

void SnapController::DispatchGetSnapHomePage(std::string snap_id,
                                             SnapHomePageCallback callback) {
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(std::nullopt, std::nullopt, "no_bridge");
    return;
  }
  bridge_controller_->LoadSnap(
      snap_id, base::BindOnce(&SnapController::OnLoadSnapForHomePage,
                              weak_ptr_factory_.GetWeakPtr(), snap_id,
                              std::move(callback)));
}

void SnapController::OnLoadSnapForHomePage(
    std::string snap_id,
    SnapHomePageCallback callback,
    bool success,
    const std::optional<std::string>& error) {
  if (!success) {
    std::move(callback).Run(std::nullopt, std::nullopt,
                            error.value_or("Failed to load snap for homepage"));
    return;
  }
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(std::nullopt, std::nullopt, "no_bridge");
    return;
  }
  bridge_controller_->FetchSnapHomePage(snap_id, std::move(callback));
}

void SnapController::SendSnapUserInput(const std::string& snap_id,
                                       const std::string& interface_id,
                                       const std::string& event_json,
                                       SnapUserInputCallback callback) {
  if (!data_provider_->IsSnapEnabled(snap_id)) {
    std::move(callback).Run(std::nullopt, "Snap is disabled");
    return;
  }
  bridge_controller_->EnsureBridgeReady(
      base::BindOnce(&SnapController::DispatchSendSnapUserInput,
                     weak_ptr_factory_.GetWeakPtr(), snap_id, interface_id,
                     event_json, std::move(callback)));
}

void SnapController::DispatchSendSnapUserInput(std::string snap_id,
                                               std::string interface_id,
                                               std::string event_json,
                                               SnapUserInputCallback callback) {
  if (!bridge_controller_->IsBound()) {
    std::move(callback).Run(std::nullopt, "no_bridge");
    return;
  }
  bridge_controller_->SendSnapUserInputEvent(snap_id, interface_id, event_json,
                                             std::move(callback));
}

// ---------------------------------------------------------------------------
// Private bridge callbacks
// ---------------------------------------------------------------------------

void SnapController::OnLoadSnapResult(size_t cb_index,
                                      std::string snap_id,
                                      std::string method,
                                      base::Value params,
                                      std::string caller_origin,
                                      bool success,
                                      const std::optional<std::string>& error) {
  if (!success) {
    FailPendingCallback(cb_index,
                        error.value_or("Failed to load snap: unknown error"));
    return;
  }
  if (!bridge_controller_->IsBound()) {
    FailPendingCallback(cb_index, "SnapBridge disconnected");
    return;
  }
  bridge_controller_->InvokeSnap(
      snap_id, method, std::move(params), std::move(caller_origin),
      base::BindOnce(&SnapController::OnInvokeSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), cb_index));
}

void SnapController::OnInvokeSnapResult(
    size_t cb_index,
    std::optional<base::Value> result,
    const std::optional<std::string>& error) {
  if (cb_index >= pending_callbacks_.size() || !pending_callbacks_[cb_index]) {
    return;
  }
  std::move(pending_callbacks_[cb_index]).Run(std::move(result), error);
}

}  // namespace brave_wallet
