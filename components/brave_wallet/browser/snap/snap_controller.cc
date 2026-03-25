/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_controller.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/snap/snap_installer.h"
#include "brave/components/brave_wallet/browser/snap/snap_registry.h"
#include "brave/components/brave_wallet/browser/snap/snap_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

// PrefService key: dict<origin_serialized, list<snap_id>>
constexpr char kConnectedSnapsPref[] = "brave.wallet.connected_snaps";

}  // namespace

SnapController::SnapController(
    KeyringService* keyring_service,
    PrefService* prefs,
    const base::FilePath& profile_path,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      prefs_(prefs),
      snap_storage_(std::make_unique<SnapStorage>(profile_path)),
      snap_installer_(std::make_unique<SnapInstaller>(
          prefs,
          snap_storage_.get(),
          std::move(url_loader_factory))),
      snap_registry_(std::make_unique<SnapRegistry>(prefs)) {
  DCHECK(keyring_service_);
  DCHECK(prefs_);
}

SnapController::~SnapController() = default;

SnapController::PendingInvoke::PendingInvoke() = default;
SnapController::PendingInvoke::PendingInvoke(PendingInvoke&&) = default;
SnapController::PendingInvoke::~PendingInvoke() = default;

// static
void SnapController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kConnectedSnapsPref);
}

void SnapController::BindSnapRequestHandler(
    mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void SnapController::SetSnapBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  LOG(ERROR) << "XXXZZZ SnapController::SetSnapBridge called, pending_callbacks="
             << pending_callbacks_.size();
  snap_bridge_.reset();
  snap_bridge_.Bind(std::move(bridge));
  snap_bridge_.set_disconnect_handler(
      base::BindOnce(&SnapController::OnSnapBridgeDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  DrainPendingInvokes();
}

void SnapController::OnSnapBridgeDisconnected() {
  LOG(ERROR) << "XXXZZZ SnapController::OnSnapBridgeDisconnected — failing "
             << pending_callbacks_.size() << " pending callback(s)";
  for (size_t i = 0; i < pending_callbacks_.size(); ++i) {
    FailPendingCallback(i, "SnapBridge disconnected");
  }
  snap_bridge_.reset();
}

void SnapController::FailPendingCallback(size_t index,
                                         const std::string& error) {
  if (index >= pending_callbacks_.size() || !pending_callbacks_[index]) {
    return;
  }
  std::move(pending_callbacks_[index]).Run(std::nullopt, error);
}

// ---------------------------------------------------------------------------
// Connection grant helpers
// ---------------------------------------------------------------------------

bool SnapController::IsSnapConnected(const url::Origin& origin,
                                     const std::string& snap_id) const {
  const base::Value::Dict& all = prefs_->GetDict(kConnectedSnapsPref);
  const std::string key = origin.Serialize();
  const base::Value::List* list = all.FindList(key);
  if (!list) {
    return false;
  }
  for (const auto& item : *list) {
    if (item.is_string() && item.GetString() == snap_id) {
      return true;
    }
  }
  return false;
}

void SnapController::GrantSnapConnection(const url::Origin& origin,
                                         const std::string& snap_id) {
  if (IsSnapConnected(origin, snap_id)) {
    return;
  }
  ScopedDictPrefUpdate update(prefs_, kConnectedSnapsPref);
  const std::string key = origin.Serialize();
  base::Value::List* list = update->FindList(key);
  if (!list) {
    update->Set(key, base::Value::List());
    list = update->FindList(key);
  }
  list->Append(snap_id);
}

void SnapController::RevokeSnapConnection(const url::Origin& origin,
                                          const std::string& snap_id) {
  ScopedDictPrefUpdate update(prefs_, kConnectedSnapsPref);
  const std::string key = origin.Serialize();
  base::Value::List* list = update->FindList(key);
  if (!list) {
    return;
  }
  list->EraseIf([&snap_id](const base::Value& v) {
    return v.is_string() && v.GetString() == snap_id;
  });
  if (list->empty()) {
    update->Remove(key);
  }
}

std::vector<std::string> SnapController::GetConnectedSnaps(
    const url::Origin& origin) const {
  std::vector<std::string> result;
  const base::Value::Dict& all = prefs_->GetDict(kConnectedSnapsPref);
  const base::Value::List* list = all.FindList(origin.Serialize());
  if (list) {
    for (const auto& item : *list) {
      if (item.is_string()) {
        result.push_back(item.GetString());
      }
    }
  }
  return result;
}

bool SnapController::IsOriginAllowedByManifest(const url::Origin& origin,
                                               const std::string& snap_id) const {
  auto manifest = snap_registry_->GetManifest(snap_id);
  if (!manifest) {
    return false;
  }
  const SnapRpcEndowment& rpc = manifest->endowment_rpc;
  if (!rpc.allow_dapps) {
    return false;
  }
  if (rpc.allowed_origins.empty()) {
    return true;  // allow_dapps=true and no origin restriction
  }
  const std::string serialized = origin.Serialize();
  return std::ranges::any_of(rpc.allowed_origins,
                              [&serialized](const std::string& o) {
                                return o == serialized;
                              });
}

bool SnapController::InvokeSnap(const std::string& snap_id,
                                const std::string& method,
                                base::Value params,
                                std::optional<url::Origin> caller_origin,
                                SnapResultCallback callback) {
  LOG(ERROR) << "XXXZZZ SnapController::InvokeSnap snap_id=" << snap_id
             << " method=" << method;

  // If the bridge is not yet connected, queue the request. The caller should
  // open brave://wallet so the bridge connects and DrainPendingInvokes() fires.
  if (!snap_bridge_.is_bound()) {
    PendingInvoke pending;
    pending.snap_id = snap_id;
    pending.method = method;
    pending.params = std::move(params);
    pending.caller_origin = std::move(caller_origin);
    pending.callback = std::move(callback);
    pending_invokes_.push_back(std::move(pending));
    return false;
  }

  if (!snap_registry_->IsKnownSnap(snap_id)) {
    LOG(ERROR) << "XXXZZZ SnapController::InvokeSnap ERROR: unknown snap " << snap_id;
    std::move(callback).Run(std::nullopt, "Unknown snap: " + snap_id);
    return true;
  }

  // Origin gate: only enforce when the call comes from a dApp (caller_origin
  // present). Internal wallet calls (nullopt) bypass the check.
  if (caller_origin.has_value()) {
    const url::Origin& origin = *caller_origin;
    if (!IsSnapConnected(origin, snap_id)) {
      std::move(callback).Run(std::nullopt,
                              "Snap is not connected to this origin");
      return true;
    }
    if (!IsOriginAllowedByManifest(origin, snap_id)) {
      std::move(callback).Run(std::nullopt,
                              "Origin not permitted by snap manifest");
      return true;
    }
  }

  const std::string origin_str =
      caller_origin ? caller_origin->Serialize() : "brave-wallet";

  // Store the callback so OnSnapBridgeDisconnected can fail it.
  const size_t cb_index = pending_callbacks_.size();
  pending_callbacks_.push_back(std::move(callback));

  snap_bridge_->LoadSnap(
      snap_id,
      base::BindOnce(&SnapController::OnLoadSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), cb_index, snap_id, method,
                     std::move(params), origin_str));
  return true;
}

void SnapController::DrainPendingInvokes() {
  std::vector<PendingInvoke> queue = std::move(pending_invokes_);
  pending_invokes_.clear();
  for (auto& p : queue) {
    InvokeSnap(p.snap_id, p.method, std::move(p.params),
               std::move(p.caller_origin), std::move(p.callback));
  }
}

bool SnapController::IsSnapAvailable(const std::string& snap_id) const {
  return snap_registry_->IsKnownSnap(snap_id);
}

std::vector<InstalledSnapInfo> SnapController::GetInstalledSnaps() const {
  return snap_installer_->GetInstalledSnaps();
}

std::optional<InstalledSnapInfo> SnapController::GetInstalledSnap(
    const std::string& snap_id) const {
  return snap_installer_->GetInstalledSnap(snap_id);
}

void SnapController::UninstallSnap(const std::string& snap_id) {
  snap_registry_->UnregisterSnap(snap_id);
  snap_installer_->UninstallSnap(snap_id);
  state_cache_.erase(snap_id);

  // Purge all connection grants for this snap across every origin.
  ScopedDictPrefUpdate update(prefs_, kConnectedSnapsPref);
  std::vector<std::string> empty_origins;
  for (auto [origin_key, value] : *update) {
    if (value.is_list()) {
      value.GetList().EraseIf([&snap_id](const base::Value& v) {
        return v.is_string() && v.GetString() == snap_id;
      });
      if (value.GetList().empty()) {
        empty_origins.push_back(origin_key);
      }
    }
  }
  for (const auto& key : empty_origins) {
    update->Remove(key);
  }
}

void SnapController::PrepareInstall(const std::string& snap_id,
                                    const std::string& version,
                                    SnapInstaller::PrepareCallback callback) {
  snap_installer_->PrepareInstall(snap_id, version, std::move(callback));
}

void SnapController::FinishInstall(const std::string& snap_id,
                                   SnapInstaller::InstallCallback callback) {
  snap_installer_->FinishInstall(
      snap_id,
      base::BindOnce(&SnapController::OnSnapInstalled,
                     weak_ptr_factory_.GetWeakPtr(), snap_id,
                     std::move(callback)));
}

void SnapController::AbortInstall(const std::string& snap_id) {
  snap_installer_->AbortInstall(snap_id);
}

void SnapController::InstallSnap(const std::string& snap_id,
                                  const std::string& version,
                                  SnapInstaller::InstallCallback callback) {
  snap_installer_->InstallSnap(
      snap_id, version,
      base::BindOnce(&SnapController::OnSnapInstalled,
                     weak_ptr_factory_.GetWeakPtr(), snap_id,
                     std::move(callback)));
}

void SnapController::GetSnapBundle(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  snap_installer_->GetSnapBundle(snap_id, std::move(cb));
}

void SnapController::GetSnapHomePage(const std::string& snap_id,
                                     SnapHomePageCallback callback) {
  if (!snap_bridge_) {
    std::move(callback).Run(std::nullopt, std::nullopt, "no_bridge");
    return;
  }
  snap_bridge_->FetchSnapHomePage(snap_id, std::move(callback));
}

void SnapController::SendSnapUserInput(const std::string& snap_id,
                                       const std::string& interface_id,
                                       const std::string& event_json,
                                       SnapUserInputCallback callback) {
  if (!snap_bridge_) {
    std::move(callback).Run(std::nullopt, "no_bridge");
    return;
  }
  snap_bridge_->SendSnapUserInputEvent(snap_id, interface_id, event_json,
                                       std::move(callback));
}

void SnapController::HandleSnapRequest(const std::string& snap_id,
                                       const std::string& method,
                                       base::Value params,
                                       HandleSnapRequestCallback callback) {
  LOG(ERROR) << "XXXZZZ SnapController::HandleSnapRequest snap_id=" << snap_id
             << " method=" << method;

  // Verify the snap has declared this permission in its manifest.
  auto manifest = snap_registry_->GetManifest(snap_id);
  if (!manifest) {
    std::move(callback).Run(std::nullopt, "Unknown snap: " + snap_id);
    return;
  }
  const auto& perms = manifest->allowed_permissions;
  // snap_confirm is the legacy alias for snap_dialog; accept either permission.
  const std::string& effective_method =
      (method == "snap_confirm") ? "snap_dialog" : method;
  if (!std::ranges::any_of(perms, [&effective_method](const std::string& p) {
        return p == effective_method;
      })) {
    std::move(callback).Run(
        std::nullopt,
        "Snap '" + snap_id + "' does not have permission to call " + method);
    return;
  }

  if (method == "snap_getBip44Entropy") {
    HandleGetBip44Entropy(snap_id, std::move(params), std::move(callback));
  } else if (method == "snap_getEntropy") {
    HandleGetEntropy(snap_id, std::move(params), std::move(callback));
  } else if (method == "snap_manageState") {
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
  } else if (method == "snap_dialog" || method == "snap_confirm") {
    // snap_confirm is the legacy name for snap_dialog with type "confirmation".
    // Both auto-confirm for now — real UI TBD.
    LOG(ERROR) << "XXXZZZ SnapController::HandleSnapRequest " << method
               << " auto-confirmed";
    std::move(callback).Run(base::Value(true), std::nullopt);
  } else if (method == "snap_notify") {
    // Notifications are fire-and-forget; acknowledge silently.
    LOG(ERROR) << "XXXZZZ SnapController::HandleSnapRequest snap_notify received";
    std::move(callback).Run(std::nullopt, std::nullopt);
  } else {
    LOG(ERROR) << "XXXZZZ SnapController::HandleSnapRequest unsupported method="
               << method;
    std::move(callback).Run(std::nullopt,
                            "Unsupported snap method: " + method);
  }
}

// Private ------------------------------------------------------------------

void SnapController::HandleGetBip44Entropy(
    const std::string& snap_id,
    base::Value params,
    HandleSnapRequestCallback callback) {
  LOG(ERROR) << "XXXZZZ SnapController::HandleGetBip44Entropy snap_id="
             << snap_id;
  // params: { "coinType": <uint32> }
  const base::Value::Dict* dict = params.GetIfDict();
  if (!dict) {
    LOG(ERROR) << "XXXZZZ SnapController::HandleGetBip44Entropy ERROR: params not a dict";
    std::move(callback).Run(std::nullopt,
                            "snap_getBip44Entropy: params must be a dict");
    return;
  }

  std::optional<int> coin_type = dict->FindInt("coinType");
  if (!coin_type || *coin_type < 0) {
    LOG(ERROR) << "XXXZZZ SnapController::HandleGetBip44Entropy ERROR: missing/invalid coinType";
    std::move(callback).Run(std::nullopt,
                            "snap_getBip44Entropy: missing or invalid coinType");
    return;
  }
  LOG(ERROR) << "XXXZZZ SnapController::HandleGetBip44Entropy coinType=" << *coin_type;

  keyring_service_->GetBip44EntropyForSnap(
      static_cast<uint32_t>(*coin_type),
      base::BindOnce(
          [](HandleSnapRequestCallback cb,
             std::optional<base::Value> result) {
            if (!result) {
              LOG(ERROR) << "XXXZZZ SnapController::HandleGetBip44Entropy ERROR: "
                            "key derivation failed (wallet locked?)";
              std::move(cb).Run(
                  std::nullopt,
                  "snap_getBip44Entropy: key derivation failed "
                  "(wallet may be locked)");
              return;
            }
            LOG(ERROR) << "XXXZZZ SnapController::HandleGetBip44Entropy: entropy derived OK";
            std::move(cb).Run(std::move(result), std::nullopt);
          },
          std::move(callback)));
}

void SnapController::HandleGetEntropy(const std::string& snap_id,
                                      base::Value params,
                                      HandleSnapRequestCallback callback) {
  std::move(callback).Run(std::nullopt, "snap_getEntropy not yet implemented");
}

void SnapController::HandleManageState(const std::string& snap_id,
                                       SnapStateOperation operation,
                                       std::string new_state_json,
                                       HandleSnapRequestCallback callback) {
  switch (operation) {
    case SnapStateOperation::kGet: {
      auto it = state_cache_.find(snap_id);
      if (it != state_cache_.end()) {
        // Return raw JSON string; renderer parses it.
        const std::string& json = it->second;
        std::move(callback).Run(
            base::Value(json.empty() ? "null" : json), std::nullopt);
        return;
      }
      snap_storage_->ReadState(
          snap_id,
          base::BindOnce(&SnapController::OnStateLoadedForGet,
                         weak_ptr_factory_.GetWeakPtr(), snap_id,
                         std::move(callback)));
      return;
    }
    case SnapStateOperation::kUpdate: {
      constexpr size_t kMaxStateBytes = 1 * 1024 * 1024;  // 1 MB
      if (new_state_json.size() > kMaxStateBytes) {
        std::move(callback).Run(std::nullopt,
                                "snap_manageState: state exceeds 1 MB limit");
        return;
      }
      state_cache_[snap_id] = new_state_json;
      snap_storage_->WriteState(
          snap_id, std::move(new_state_json),
          base::BindOnce([](bool success) {
            if (!success) {
              LOG(ERROR) << "snap_manageState: failed to persist state";
            }
          }));
      std::move(callback).Run(base::Value("null"), std::nullopt);
      return;
    }
    case SnapStateOperation::kClear: {
      state_cache_[snap_id] = "";
      snap_storage_->WriteState(
          snap_id, "{}",
          base::BindOnce([](bool success) {
            if (!success) {
              LOG(ERROR) << "snap_manageState: failed to clear state";
            }
          }));
      std::move(callback).Run(base::Value("null"), std::nullopt);
      return;
    }
  }
}

void SnapController::OnStateLoadedForGet(
    std::string snap_id,
    HandleSnapRequestCallback callback,
    std::optional<std::string> disk_json) {
  if (!disk_json || disk_json->empty()) {
    state_cache_[snap_id] = "";
    std::move(callback).Run(base::Value("null"), std::nullopt);
    return;
  }
  state_cache_[snap_id] = *disk_json;
  std::move(callback).Run(base::Value(std::move(*disk_json)), std::nullopt);
}

void SnapController::OnSnapInstalled(std::string snap_id,
                                      SnapInstaller::InstallCallback callback,
                                      bool success,
                                      const std::string& error) {
  if (success) {
    // Sync the in-memory registry with the newly persisted snap metadata.
    auto info = snap_installer_->GetInstalledSnap(snap_id);
    if (info) {
      snap_registry_->RegisterInstalledSnap(snap_id, info->version,
                                             info->permissions,
                                             info->endowment_rpc);
    }
  }
  std::move(callback).Run(success, error);
}

void SnapController::OnLoadSnapResult(
    size_t cb_index,
    const std::string& snap_id,
    const std::string& method,
    base::Value params,
    std::string caller_origin,
    bool success,
    const std::optional<std::string>& error) {
  LOG(ERROR) << "XXXZZZ SnapController::OnLoadSnapResult snap_id=" << snap_id
             << " method=" << method << " success=" << success
             << (error ? " error=" + *error : "");
  if (!success) {
    LOG(ERROR) << "XXXZZZ SnapController::OnLoadSnapResult ERROR: "
               << error.value_or("unknown error");
    FailPendingCallback(
        cb_index, error.value_or("Failed to load snap: unknown error"));
    return;
  }

  if (!snap_bridge_.is_bound()) {
    LOG(ERROR) << "XXXZZZ SnapController::OnLoadSnapResult ERROR: "
                  "SnapBridge disconnected before InvokeSnap";
    FailPendingCallback(cb_index, "SnapBridge disconnected");
    return;
  }

  snap_bridge_->InvokeSnap(
      snap_id, method, std::move(params), std::move(caller_origin),
      base::BindOnce(&SnapController::OnInvokeSnapResult,
                     weak_ptr_factory_.GetWeakPtr(), cb_index));
}

void SnapController::OnInvokeSnapResult(
    size_t cb_index,
    std::optional<base::Value> result,
    const std::optional<std::string>& error) {
  LOG(ERROR) << "XXXZZZ SnapController::OnInvokeSnapResult"
             << " has_result=" << result.has_value()
             << (error ? " error=" + *error : "");
  if (cb_index >= pending_callbacks_.size() || !pending_callbacks_[cb_index]) {
    return;  // Already failed by OnSnapBridgeDisconnected.
  }
  std::move(pending_callbacks_[cb_index]).Run(std::move(result), error);
}

}  // namespace brave_wallet
