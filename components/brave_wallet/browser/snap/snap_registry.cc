/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_registry.h"

#include "base/time/time.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

constexpr char kInstalledSnapsPref[] = "brave.wallet.installed_snaps";

base::Value::List PermissionsToList(const std::vector<std::string>& perms) {
  base::Value::List list;
  for (const auto& p : perms) {
    list.Append(p);
  }
  return list;
}

// Serializes mojom::SnapManifest fields into a pref sub-dict.
base::Value::Dict ManifestToDict(const mojom::SnapManifest& manifest) {
  base::Value::Dict dict;
  dict.Set("proposed_name", manifest.proposed_name);
  dict.Set("description", manifest.description);
  dict.Set("permissions", PermissionsToList(manifest.permissions));

  base::Value::Dict rpc;
  rpc.Set("dapps", manifest.allow_dapps);
  rpc.Set("snaps", manifest.allow_snaps);
  base::Value::List origins;
  for (const auto& o : manifest.allowed_rpc_origins) {
    origins.Append(o);
  }
  rpc.Set("allowedOrigins", std::move(origins));
  dict.Set("rpc", std::move(rpc));
  return dict;
}

// Reads a SnapInstallData from a single snap pref entry (outer dict keyed by
// snap_id).
mojom::SnapInstallDataPtr SnapFromPref(const std::string& snap_id,
                                        const base::Value::Dict& outer) {
  auto install_data = mojom::SnapInstallData::New();
  install_data->snap_id = snap_id;
  install_data->version = outer.FindString("version")
                               ? *outer.FindString("version")
                               : std::string();
  install_data->bundle_size_bytes =
      static_cast<uint64_t>(outer.FindDouble("bundle_size_bytes").value_or(0));
  install_data->enabled = outer.FindBool("enabled").value_or(true);

  auto manifest = mojom::SnapManifest::New();
  const base::Value::Dict* m = outer.FindDict("manifest");
  if (m) {
    if (const std::string* v = m->FindString("proposed_name")) {
      manifest->proposed_name = *v;
    }
    if (const std::string* v = m->FindString("description")) {
      manifest->description = *v;
    }
    if (const base::Value::List* list = m->FindList("permissions")) {
      for (const auto& item : *list) {
        if (item.is_string()) {
          manifest->permissions.push_back(item.GetString());
        }
      }
    }
    if (const base::Value::Dict* rpc = m->FindDict("rpc")) {
      manifest->allow_dapps = rpc->FindBool("dapps").value_or(false);
      manifest->allow_snaps = rpc->FindBool("snaps").value_or(false);
      if (const base::Value::List* origins = rpc->FindList("allowedOrigins")) {
        for (const auto& item : *origins) {
          if (item.is_string()) {
            manifest->allowed_rpc_origins.push_back(item.GetString());
          }
        }
      }
    }
  }
  install_data->manifest = std::move(manifest);
  return install_data;
}

}  // namespace

// static
void SnapRegistry::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kInstalledSnapsPref);
}

SnapRegistry::SnapRegistry(PrefService& prefs) : prefs_(prefs) {
  const base::Value::Dict& all = prefs_->GetDict(kInstalledSnapsPref);
  for (const auto [snap_id, value] : all) {
    if (value.is_dict()) {
      installed_snaps_[snap_id] = SnapFromPref(snap_id, value.GetDict());
    }
  }
}

SnapRegistry::~SnapRegistry() = default;

mojom::SnapInstallDataPtr SnapRegistry::GetSnap(
    const std::string& snap_id) const {
  auto it = installed_snaps_.find(snap_id);
  if (it == installed_snaps_.end()) {
    return nullptr;
  }
  return it->second->Clone();
}

bool SnapRegistry::IsKnownSnap(const std::string& snap_id) const {
  return installed_snaps_.contains(snap_id);
}

std::vector<mojom::SnapInstallDataPtr> SnapRegistry::GetAllSnaps() const {
  std::vector<mojom::SnapInstallDataPtr> result;
  result.reserve(installed_snaps_.size());
  for (const auto& [_, snap] : installed_snaps_) {
    result.push_back(snap->Clone());
  }
  return result;
}

void SnapRegistry::RegisterInstalledSnap(
    const mojom::SnapInstallData& install_data) {
  DCHECK(install_data.manifest);
  {
    ScopedDictPrefUpdate update(&*prefs_, kInstalledSnapsPref);
    base::Value::Dict snap_info;
    snap_info.Set("version", install_data.version);
    snap_info.Set("bundle_size_bytes",
                  static_cast<double>(install_data.bundle_size_bytes));
    snap_info.Set("manifest", ManifestToDict(*install_data.manifest));
    snap_info.Set("installed_at",
                  base::Time::Now().InSecondsFSinceUnixEpoch());
    snap_info.Set("enabled", install_data.enabled);
    update->Set(install_data.snap_id, std::move(snap_info));
  }
  installed_snaps_[install_data.snap_id] = install_data.Clone();
}

void SnapRegistry::UnregisterSnap(const std::string& snap_id) {
  ScopedDictPrefUpdate update(&*prefs_, kInstalledSnapsPref);
  update->Remove(snap_id);
  installed_snaps_.erase(snap_id);
}

}  // namespace brave_wallet
