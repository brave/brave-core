/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/snap_manifest_helpers.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

constexpr char kInstalledSnapsPref[] = "brave.wallet.installed_snaps";

// Serializes mojom::SnapManifest fields into a pref sub-dict.
base::DictValue ManifestToDict(const mojom::SnapManifest& manifest) {
  return SnapManifestToValue(manifest);
}

// Reads a SnapInstallData from a single snap pref entry (outer dict keyed by
// snap_id).
mojom::SnapInstallDataPtr SnapFromPref(const std::string& snap_id,
                                       const base::DictValue& outer) {
  const std::string* version = outer.FindString("version");
  const std::optional<double> bundle_size_bytes =
      outer.FindDouble("bundle_size_bytes");
  const std::optional<bool> enabled = outer.FindBool("enabled");
  const std::string* install_origin = outer.FindString("install_origin");
  const base::DictValue* manifest_dict = outer.FindDict("manifest");
  const std::string* description = outer.FindString("description");
  if (!version || !bundle_size_bytes || !enabled || !install_origin ||
      !manifest_dict || !description) {
    return nullptr;
  }

  auto install_data = mojom::SnapInstallData::New();
  install_data->snap_id = snap_id;
  install_data->version = *version;
  install_data->bundle_size_bytes =
      static_cast<uint64_t>(*bundle_size_bytes);
  install_data->enabled = *enabled;
  install_data->install_origin = *install_origin;
  install_data->manifest = SnapManifestFromValue(*manifest_dict);
  install_data->description = *description;
  return install_data;
}

}  // namespace

// static
void SnapRegistry::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kInstalledSnapsPref);
}

SnapRegistry::SnapRegistry(PrefService& prefs) : prefs_(prefs) {}

SnapRegistry::~SnapRegistry() = default;

void SnapRegistry::EnsureInstalledSnapsLoaded() const {
  if (!installed_snaps_.empty()) {
    return;
  }
  const base::DictValue& all = prefs_->GetDict(kInstalledSnapsPref);
  for (const auto [snap_id, value] : all) {
    if (!value.is_dict()) {
      continue;
    }
    mojom::SnapInstallDataPtr snap = SnapFromPref(snap_id, value.GetDict());
    if (snap) {
      installed_snaps_[snap_id] = std::move(snap);
    }
  }
}

void SnapRegistry::ResetInstalledSnaps() {
  installed_snaps_.clear();
}

mojom::SnapInstallDataPtr SnapRegistry::GetSnap(
    const std::string& snap_id) const {
  EnsureInstalledSnapsLoaded();
  auto it = installed_snaps_.find(snap_id);
  if (it == installed_snaps_.end()) {
    return nullptr;
  }
  return it->second->Clone();
}

bool SnapRegistry::IsKnownSnap(const std::string& snap_id) const {
  EnsureInstalledSnapsLoaded();
  return installed_snaps_.contains(snap_id);
}

bool SnapRegistry::IsSnapEnabled(const std::string& snap_id) const {
  EnsureInstalledSnapsLoaded();
  auto it = installed_snaps_.find(snap_id);
  if (it == installed_snaps_.end()) {
    return true;
  }
  return it->second->enabled;
}

std::vector<mojom::SnapInstallDataPtr> SnapRegistry::GetAllSnaps() const {
  EnsureInstalledSnapsLoaded();
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
  const bool enabled = install_data.enabled;
  {
    ScopedDictPrefUpdate update(&*prefs_, kInstalledSnapsPref);
    base::DictValue snap_info;
    snap_info.Set("version", install_data.version);
    snap_info.Set("bundle_size_bytes",
                  static_cast<double>(install_data.bundle_size_bytes));
    snap_info.Set("manifest", ManifestToDict(*install_data.manifest));
    snap_info.Set("description", install_data.description);
    snap_info.Set("install_origin", install_data.install_origin);
    snap_info.Set("installed_at", base::Time::Now().InSecondsFSinceUnixEpoch());
    snap_info.Set("enabled", enabled);
    update->Set(install_data.snap_id, std::move(snap_info));
  }
  ResetInstalledSnaps();
}

void SnapRegistry::UnregisterSnap(const std::string& snap_id) {
  ScopedDictPrefUpdate update(&*prefs_, kInstalledSnapsPref);
  update->Remove(snap_id);
  ResetInstalledSnaps();
}

void SnapRegistry::SetSnapEnabled(const std::string& snap_id, bool enabled) {
  EnsureInstalledSnapsLoaded();
  auto it = installed_snaps_.find(snap_id);
  if (it == installed_snaps_.end() || it->second->enabled == enabled) {
    return;
  }
  ScopedDictPrefUpdate update(&*prefs_, kInstalledSnapsPref);
  base::DictValue* snap_dict = update->FindDict(snap_id);
  if (snap_dict) {
    snap_dict->Set("enabled", enabled);
  }
  ResetInstalledSnaps();
}

}  // namespace brave_wallet
