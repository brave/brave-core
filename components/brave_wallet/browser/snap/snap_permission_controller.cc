/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/snap/snap_manifest_helpers.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

// dict<origin_serialized, list<snap_id>>
constexpr char kConnectedSnapsPref[] = "brave.wallet.connected_snaps";

// snap_confirm is the legacy alias for snap_dialog; both map to the
// "snap_dialog" permission entry in the manifest.
constexpr char kMethodDialog[] = "snap_dialog";
constexpr char kMethodConfirm[] = "snap_confirm";

bool IsOriginAllowedBySnapManifest(const url::Origin& origin,
                                   const mojom::SnapInstallDataPtr& snap) {
  if (!snap || !snap->manifest) {
    return false;
  }
  return SnapManifestAllowsOrigin(*snap->manifest, origin);
}

}  // namespace

SnapPermissionController::SnapPermissionController(
    PrefService& prefs,
    SnapDataProvider& data_provider)
    : prefs_(prefs), data_provider_(data_provider) {}

SnapPermissionController::~SnapPermissionController() = default;

// static
void SnapPermissionController::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kConnectedSnapsPref);
}

bool SnapPermissionController::IsSnapConnected(
    const url::Origin& origin,
    const std::string& snap_id) const {
  const base::DictValue& all = prefs_->GetDict(kConnectedSnapsPref);
  const base::ListValue* list = all.FindList(origin.Serialize());
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

void SnapPermissionController::GrantSnapConnection(const url::Origin& origin,
                                                   const std::string& snap_id) {
  if (IsSnapConnected(origin, snap_id)) {
    return;
  }
  ScopedDictPrefUpdate update(&*prefs_, kConnectedSnapsPref);
  const std::string key = origin.Serialize();
  base::ListValue* list = update->FindList(key);
  if (!list) {
    update->Set(key, base::ListValue());
    list = update->FindList(key);
  }
  list->Append(snap_id);
}

void SnapPermissionController::RevokeSnapConnection(
    const url::Origin& origin,
    const std::string& snap_id) {
  ScopedDictPrefUpdate update(&*prefs_, kConnectedSnapsPref);
  const std::string key = origin.Serialize();
  base::ListValue* list = update->FindList(key);
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

std::vector<std::string> SnapPermissionController::GetConnectedSnaps(
    const url::Origin& origin) const {
  std::vector<std::string> result;
  const base::DictValue& all = prefs_->GetDict(kConnectedSnapsPref);
  const base::ListValue* list = all.FindList(origin.Serialize());
  if (list) {
    for (const auto& item : *list) {
      if (item.is_string()) {
        result.push_back(item.GetString());
      }
    }
  }
  return result;
}

std::vector<std::string> SnapPermissionController::GetOriginsConnectedToSnap(
    const std::string& snap_id) const {
  std::vector<std::string> result;
  const base::DictValue& all = prefs_->GetDict(kConnectedSnapsPref);
  for (const auto [origin_key, value] : all) {
    if (!value.is_list()) {
      continue;
    }
    for (const auto& item : value.GetList()) {
      if (item.is_string() && item.GetString() == snap_id) {
        result.push_back(origin_key);
        break;
      }
    }
  }
  return result;
}

void SnapPermissionController::IsOriginAllowedByManifest(
    const url::Origin& origin,
    const std::string& snap_id,
    base::OnceCallback<void(bool)> callback) const {
  data_provider_->GetSnap(
      snap_id,
      base::BindOnce(&SnapPermissionController::OnOriginAllowedByManifestChecked,
                     weak_ptr_factory_.GetWeakPtr(), origin,
                     std::move(callback)));
}

void SnapPermissionController::OnOriginAllowedByManifestChecked(
    url::Origin origin,
    base::OnceCallback<void(bool)> callback,
    mojom::SnapInstallDataPtr snap) {
  std::move(callback).Run(IsOriginAllowedBySnapManifest(origin, snap));
}

void SnapPermissionController::CheckSnapMethodPermission(
    const std::string& snap_id,
    const std::string& method,
    base::OnceCallback<void(std::optional<std::string>)> callback) const {
  data_provider_->GetSnap(
      snap_id,
      base::BindOnce(
          &SnapPermissionController::OnCheckSnapMethodPermissionSnapLoaded,
          weak_ptr_factory_.GetWeakPtr(), snap_id, method,
          std::move(callback)));
}

void SnapPermissionController::OnCheckSnapMethodPermissionSnapLoaded(
    std::string snap_id,
    std::string method,
    base::OnceCallback<void(std::optional<std::string>)> callback,
    mojom::SnapInstallDataPtr snap) {
  if (!snap || !snap->manifest) {
    std::move(callback).Run("Unknown snap: " + snap_id);
    return;
  }
  // snap_confirm is the legacy alias; the manifest declares "snap_dialog".
  const std::string& effective =
      (method == kMethodConfirm) ? kMethodDialog : method;
  if (!SnapManifestHasPermission(*snap->manifest, effective)) {
    std::move(callback).Run("Snap '" + snap_id +
                            "' does not have permission to call " + method);
    return;
  }
  std::move(callback).Run(std::nullopt);
}

void SnapPermissionController::PurgeConnectionGrantsForSnap(
    const std::string& snap_id) {
  ScopedDictPrefUpdate update(&*prefs_, kConnectedSnapsPref);
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

}  // namespace brave_wallet
