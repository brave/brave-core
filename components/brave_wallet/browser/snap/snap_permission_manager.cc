/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_permission_manager.h"

#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {
// Pref key: dict<origin_string, dict<snap_id, bool>>
constexpr char kSnapPermissionsPref[] = "brave.wallet.snap_permissions";
}  // namespace

SnapPermissionManager::SnapPermissionManager(PrefService* prefs)
    : prefs_(prefs) {}

SnapPermissionManager::~SnapPermissionManager() = default;

// static
void SnapPermissionManager::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kSnapPermissionsPref);
}

bool SnapPermissionManager::HasPermission(const url::Origin& origin,
                                          const std::string& snap_id) const {
  const base::Value::Dict& all = prefs_->GetDict(kSnapPermissionsPref);
  const std::string origin_key = origin.Serialize();
  const base::Value::Dict* origin_dict = all.FindDict(origin_key);
  if (!origin_dict) {
    return false;
  }
  std::optional<bool> granted = origin_dict->FindBool(snap_id);
  return granted.value_or(false);
}

void SnapPermissionManager::GrantPermission(const url::Origin& origin,
                                            const std::string& snap_id) {
  ScopedDictPrefUpdate update(prefs_, kSnapPermissionsPref);
  const std::string origin_key = origin.Serialize();
  base::Value::Dict* origin_dict = update->FindDict(origin_key);
  if (!origin_dict) {
    update->Set(origin_key, base::Value::Dict());
    origin_dict = update->FindDict(origin_key);
  }
  origin_dict->Set(snap_id, true);
}

void SnapPermissionManager::RevokePermission(const url::Origin& origin,
                                              const std::string& snap_id) {
  ScopedDictPrefUpdate update(prefs_, kSnapPermissionsPref);
  const std::string origin_key = origin.Serialize();
  base::Value::Dict* origin_dict = update->FindDict(origin_key);
  if (origin_dict) {
    origin_dict->Remove(snap_id);
  }
}

void SnapPermissionManager::RevokeAllPermissions(const url::Origin& origin) {
  ScopedDictPrefUpdate update(prefs_, kSnapPermissionsPref);
  update->Remove(origin.Serialize());
}

}  // namespace brave_wallet
