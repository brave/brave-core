/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/snap/snap_data_provider.h"
#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"

namespace brave_wallet {

namespace {

constexpr char kMethodGetBip44Entropy[] = "snap_getBip44Entropy";
constexpr char kMethodGetEntropy[]      = "snap_getEntropy";
constexpr char kMethodManageState[]     = "snap_manageState";
constexpr char kMethodDialog[]          = "snap_dialog";
constexpr char kMethodConfirm[]         = "snap_confirm";
constexpr char kMethodNotify[]          = "snap_notify";

}  // namespace

SnapRequestHandlerImpl::SnapRequestHandlerImpl(
    KeyringService& keyring_service,
    SnapDataProvider& data_provider,
    SnapPermissionController& permission_controller)
    : keyring_service_(keyring_service),
      data_provider_(data_provider),
      permission_controller_(permission_controller) {}

SnapRequestHandlerImpl::~SnapRequestHandlerImpl() = default;

void SnapRequestHandlerImpl::Bind(
    mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void SnapRequestHandlerImpl::HandleSnapRequest(
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    HandleSnapRequestCallback callback) {
  if (auto error =
          permission_controller_->CheckSnapMethodPermission(snap_id, method)) {
    std::move(callback).Run(std::nullopt, std::move(*error));
    return;
  }

  if (method == kMethodGetBip44Entropy) {
    HandleGetBip44Entropy(snap_id, std::move(params), std::move(callback));
  } else if (method == kMethodGetEntropy) {
    HandleGetEntropy(snap_id, std::move(params), std::move(callback));
  } else if (method == kMethodManageState) {
    const base::Value::Dict* dict = params.GetIfDict();
    const std::string* op_str = dict ? dict->FindString("operation") : nullptr;
    if (!op_str) {
      std::move(callback).Run(std::nullopt,
                              "snap_manageState: missing operation");
      return;
    }
    SnapStateOperation op;
    std::string new_state_json;
    if (*op_str == "get") {
      op = SnapStateOperation::kGet;
    } else if (*op_str == "update") {
      op = SnapStateOperation::kUpdate;
      const std::string* json = dict->FindString("newStateJson");
      if (!json) {
        std::move(callback).Run(std::nullopt,
                                "snap_manageState: missing newStateJson");
        return;
      }
      new_state_json = *json;
    } else if (*op_str == "clear") {
      op = SnapStateOperation::kClear;
    } else {
      std::move(callback).Run(
          std::nullopt, "snap_manageState: unknown operation: " + *op_str);
      return;
    }
    HandleManageState(snap_id, op, std::move(new_state_json),
                      std::move(callback));
  } else if (method == kMethodDialog || method == kMethodConfirm) {
    std::move(callback).Run(base::Value(true), std::nullopt);
  } else if (method == kMethodNotify) {
    std::move(callback).Run(std::nullopt, std::nullopt);
  } else {
    std::move(callback).Run(std::nullopt,
                            "Unsupported snap method: " + method);
  }
}

void SnapRequestHandlerImpl::HandleGetBip44Entropy(
    const std::string& snap_id,
    base::Value params,
    HandleSnapRequestCallback callback) {
  const base::Value::Dict* dict = params.GetIfDict();
  if (!dict) {
    std::move(callback).Run(
        std::nullopt,
        std::string(kMethodGetBip44Entropy) + ": params must be a dict");
    return;
  }
  std::optional<int> coin_type = dict->FindInt("coinType");
  if (!coin_type || *coin_type < 0) {
    std::move(callback).Run(
        std::nullopt,
        std::string(kMethodGetBip44Entropy) + ": missing or invalid coinType");
    return;
  }
  keyring_service_->GetBip44EntropyForSnap(
      static_cast<uint32_t>(*coin_type),
      base::BindOnce(
          [](HandleSnapRequestCallback cb,
             std::optional<base::Value> result) {
            if (!result) {
              std::move(cb).Run(
                  std::nullopt,
                  std::string(kMethodGetBip44Entropy) +
                      ": key derivation failed (wallet may be locked)");
              return;
            }
            std::move(cb).Run(std::move(result), std::nullopt);
          },
          std::move(callback)));
}

void SnapRequestHandlerImpl::HandleGetEntropy(
    const std::string& snap_id,
    base::Value params,
    HandleSnapRequestCallback callback) {
  std::move(callback).Run(
      std::nullopt, std::string(kMethodGetEntropy) + " not yet implemented");
}

void SnapRequestHandlerImpl::HandleManageState(
    const std::string& snap_id,
    SnapStateOperation operation,
    std::string new_state_json,
    HandleSnapRequestCallback callback) {
  switch (operation) {
    case SnapStateOperation::kGet:
      data_provider_->GetSnapState(
          snap_id,
          base::BindOnce(
              [](HandleSnapRequestCallback cb,
                 std::optional<std::string> json) {
                std::move(cb).Run(
                    base::Value(json.value_or("null")), std::nullopt);
              },
              std::move(callback)));
      return;

    case SnapStateOperation::kUpdate:
      data_provider_->UpdateSnapState(
          snap_id, std::move(new_state_json),
          base::BindOnce(
              [](HandleSnapRequestCallback cb,
                 std::optional<std::string> error) {
                if (error) {
                  std::move(cb).Run(std::nullopt, std::move(*error));
                } else {
                  std::move(cb).Run(base::Value("null"), std::nullopt);
                }
              },
              std::move(callback)));
      return;

    case SnapStateOperation::kClear:
      data_provider_->ClearSnapState(
          snap_id,
          base::BindOnce(
              [](HandleSnapRequestCallback cb,
                 std::optional<std::string> /*error*/) {
                std::move(cb).Run(base::Value("null"), std::nullopt);
              },
              std::move(callback)));
      return;
  }
}

}  // namespace brave_wallet
