/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_permission_controller.h"

#include <algorithm>
#include <utility>

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
constexpr char kMethodDialog[]  = "snap_dialog";
constexpr char kMethodConfirm[] = "snap_confirm";

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

void SnapPermissionController::GrantSnapConnection(
    const url::Origin& origin,
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

bool SnapPermissionController::IsOriginAllowedByManifest(
    const url::Origin& origin,
    const std::string& snap_id) const {
  auto snap = data_provider_->GetSnap(snap_id);
  if (!snap || !snap->manifest) {
    return false;
  }
  if (snap->manifest->allow_dapps) {
    return true;
  }
  if (snap->manifest->allowed_rpc_origins.empty()) {
    return false;
  }
  const std::string serialized = origin.Serialize();
  return std::ranges::any_of(snap->manifest->allowed_rpc_origins,
                              [&serialized](const std::string& o) {
                                return o == serialized;
                              });
}

std::optional<std::string> SnapPermissionController::CheckSnapMethodPermission(
    const std::string& snap_id,
    const std::string& method) const {
  auto snap = data_provider_->GetSnap(snap_id);
  if (!snap || !snap->manifest) {
    return "Unknown snap: " + snap_id;
  }
  // snap_confirm is the legacy alias; the manifest declares "snap_dialog".
  const std::string& effective =
      (method == kMethodConfirm) ? kMethodDialog : method;
  const auto& perms = snap->manifest->permissions;
  if (!std::ranges::any_of(perms,
                            [&effective](const std::string& p) {
                              return p == effective;
                            })) {
    return "Snap '" + snap_id + "' does not have permission to call " + method;
  }
  return std::nullopt;
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
