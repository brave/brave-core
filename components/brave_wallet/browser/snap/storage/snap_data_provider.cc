/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_storage.h"

namespace brave_wallet {

SnapDataProvider::SnapDataProvider(const base::FilePath& profile_path,
                                   PrefService& prefs)
    : snap_storage_(std::make_unique<SnapStorage>(profile_path)),
      snap_registry_(std::make_unique<SnapRegistry>(prefs)) {}

SnapDataProvider::~SnapDataProvider() = default;

// ---------------------------------------------------------------------------
// Registry queries
// ---------------------------------------------------------------------------

bool SnapDataProvider::IsInstalled(const std::string& snap_id) const {
  return snap_registry_->IsKnownSnap(snap_id);
}

bool SnapDataProvider::IsSnapEnabled(const std::string& snap_id) const {
  auto snap = snap_registry_->GetSnap(snap_id);
  return snap && snap->enabled;
}

mojom::SnapInstallDataPtr SnapDataProvider::GetSnap(
    const std::string& snap_id) const {
  return snap_registry_->GetSnap(snap_id);
}

std::vector<mojom::SnapInstallDataPtr> SnapDataProvider::GetAllSnaps() const {
  return snap_registry_->GetAllSnaps();
}

void SnapDataProvider::SetSnapEnabled(const std::string& snap_id,
                                      bool enabled) {
  snap_registry_->SetSnapEnabled(snap_id, enabled);
}

// ---------------------------------------------------------------------------
// Bundle I/O
// ---------------------------------------------------------------------------

void SnapDataProvider::SaveBundleFromDir(const std::string& snap_id,
                                          const base::FilePath& unpacked_dir,
                                          base::OnceCallback<void(bool)> on_done) {
  snap_storage_->MoveSnapFiles(snap_id, unpacked_dir, std::move(on_done));
}

void SnapDataProvider::ReadBundle(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  snap_storage_->ReadBundle(snap_id, std::move(cb));
}

// ---------------------------------------------------------------------------
// Snap state
// ---------------------------------------------------------------------------

void SnapDataProvider::GetSnapState(const std::string& snap_id,
                                     StateCallback callback) {
  auto it = state_cache_.find(snap_id);
  if (it != state_cache_.end()) {
    const std::string& json = it->second;
    std::move(callback).Run(json.empty() ? std::nullopt
                                         : std::make_optional(json));
    return;
  }
  snap_storage_->ReadState(
      snap_id,
      base::BindOnce(&SnapDataProvider::OnStateLoaded,
                     weak_ptr_factory_.GetWeakPtr(), snap_id,
                     std::move(callback)));
}

void SnapDataProvider::UpdateSnapState(const std::string& snap_id,
                                        std::string json,
                                        StateWriteCallback on_done) {
  constexpr size_t kMaxStateBytes = 1 * 1024 * 1024;  // 1 MB
  if (json.size() > kMaxStateBytes) {
    std::move(on_done).Run("snap_manageState: state exceeds 1 MB limit");
    return;
  }
  state_cache_[snap_id] = json;
  // Persist state to disk and complete |on_done| when the write finishes.
  snap_storage_->WriteState(
      snap_id, std::move(json),
      base::BindOnce(&SnapDataProvider::OnStatePersisted, std::move(on_done)));
}

void SnapDataProvider::ClearSnapState(const std::string& snap_id,
                                       StateWriteCallback on_done) {
  state_cache_[snap_id] = "";
  snap_storage_->WriteState(
      snap_id, "{}",
      base::BindOnce(&SnapDataProvider::OnStateCleared, std::move(on_done)));
}

// ---------------------------------------------------------------------------
// Install lifecycle
// ---------------------------------------------------------------------------

void SnapDataProvider::OnSnapInstalled(
    const mojom::SnapInstallData& install_data) {
  snap_registry_->RegisterInstalledSnap(install_data);
}

void SnapDataProvider::OnSnapUninstalled(const std::string& snap_id) {
  snap_registry_->UnregisterSnap(snap_id);
  state_cache_.erase(snap_id);
  snap_storage_->DeleteSnap(snap_id);
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void SnapDataProvider::OnStatePersisted(StateWriteCallback on_done,
                                        bool success) {
  if (!success) {
    DLOG(ERROR) << "snap_manageState: failed to persist state";
    std::move(on_done).Run("snap_manageState: failed to persist state");
    return;
  }
  std::move(on_done).Run(std::nullopt);
}

void SnapDataProvider::OnStateCleared(StateWriteCallback on_done,
                                      bool success) {
  if (!success) {
    DLOG(ERROR) << "snap_manageState: failed to clear state";
    std::move(on_done).Run("snap_manageState: failed to clear state");
    return;
  }
  std::move(on_done).Run(std::nullopt);
}

void SnapDataProvider::OnStateLoaded(std::string snap_id,
                                      StateCallback callback,
                                      std::optional<std::string> disk_json) {
  if (!disk_json || disk_json->empty()) {
    state_cache_[snap_id] = "";
    std::move(callback).Run(std::nullopt);
    return;
  }
  state_cache_[snap_id] = *disk_json;
  std::move(callback).Run(std::move(disk_json));
}

}  // namespace brave_wallet
