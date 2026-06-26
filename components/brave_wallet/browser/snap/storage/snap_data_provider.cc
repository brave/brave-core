/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_storage.h"

namespace brave_wallet {

SnapDataProvider::SnapDataProvider(const base::FilePath& profile_path,
                                   PrefService& prefs)
    : snap_storage_(base::ThreadPool::CreateSequencedTaskRunner(
                        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
                         base::TaskShutdownBehavior::BLOCK_SHUTDOWN}),
                    profile_path.Append(FILE_PATH_LITERAL("BraveWallet"))
                        .Append(FILE_PATH_LITERAL("Snaps")),
                    base::SequencedTaskRunner::GetCurrentDefault()),
      snap_registry_(std::make_unique<SnapRegistry>(prefs)) {}

SnapDataProvider::~SnapDataProvider() = default;

// ---------------------------------------------------------------------------
// Registry queries
// ---------------------------------------------------------------------------

void SnapDataProvider::IsInstalled(const std::string& snap_id,
                                   BoolCallback callback) {
  PostBoolCallback(std::move(callback), snap_registry_->IsKnownSnap(snap_id));
}

void SnapDataProvider::IsSnapEnabled(const std::string& snap_id,
                                     BoolCallback callback) {
  PostBoolCallback(std::move(callback), snap_registry_->IsSnapEnabled(snap_id));
}

void SnapDataProvider::GetSnap(const std::string& snap_id,
                               SnapCallback callback) {
  PostSnapCallback(std::move(callback), snap_registry_->GetSnap(snap_id));
}

void SnapDataProvider::GetAllSnaps(SnapsCallback callback) {
  PostSnapsCallback(std::move(callback), snap_registry_->GetAllSnaps());
}

void SnapDataProvider::SetSnapEnabled(const std::string& snap_id,
                                      bool enabled,
                                      BoolCallback callback) {
  const bool is_installed = snap_registry_->IsKnownSnap(snap_id);
  snap_registry_->SetSnapEnabled(snap_id, enabled);
  PostBoolCallback(std::move(callback), is_installed);
}

// ---------------------------------------------------------------------------
// Bundle I/O
// ---------------------------------------------------------------------------

void SnapDataProvider::SaveBundleFromDir(
    const std::string& snap_id,
    const base::FilePath& unpacked_dir,
    base::OnceCallback<void(bool)> on_done) {
  snap_storage_.AsyncCall(&SnapStorage::MoveSnapFiles)
      .WithArgs(snap_id, unpacked_dir)
      .Then(std::move(on_done));
}

void SnapDataProvider::ReadBundle(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  snap_storage_.AsyncCall(&SnapStorage::ReadBundle)
      .WithArgs(snap_id)
      .Then(std::move(cb));
}

// ---------------------------------------------------------------------------
// Snap state
// ---------------------------------------------------------------------------

void SnapDataProvider::GetSnapState(const std::string& snap_id,
                                    StateCallback callback) {
  auto it = state_cache_.find(snap_id);
  if (it != state_cache_.end()) {
    std::optional<std::string> json =
        it->second.empty() ? std::nullopt : std::make_optional(it->second);
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(json)));
    return;
  }
  snap_storage_.AsyncCall(&SnapStorage::ReadState)
      .WithArgs(snap_id)
      .Then(base::BindOnce(&SnapDataProvider::OnStateLoaded,
                           weak_ptr_factory_.GetWeakPtr(), snap_id,
                           std::move(callback)));
}

void SnapDataProvider::UpdateSnapState(const std::string& snap_id,
                                       std::string json,
                                       StateWriteCallback on_done) {
  constexpr size_t kMaxStateBytes = 1 * 1024 * 1024;  // 1 MB
  if (json.size() > kMaxStateBytes) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(on_done),
                                  std::make_optional<std::string>(
                                      "snap_manageState: state exceeds 1 MB "
                                      "limit")));
    return;
  }
  state_cache_[snap_id] = json;
  // Persist state to disk and complete |on_done| when the write finishes.
  snap_storage_.AsyncCall(&SnapStorage::WriteState)
      .WithArgs(snap_id, std::move(json))
      .Then(base::BindOnce(&SnapDataProvider::OnStatePersisted,
                           std::move(on_done)));
}

void SnapDataProvider::ClearSnapState(const std::string& snap_id,
                                      StateWriteCallback on_done) {
  state_cache_[snap_id] = "";
  snap_storage_.AsyncCall(&SnapStorage::WriteState)
      .WithArgs(snap_id, "{}")
      .Then(
          base::BindOnce(&SnapDataProvider::OnStateCleared, std::move(on_done)));
}

// ---------------------------------------------------------------------------
// Install lifecycle
// ---------------------------------------------------------------------------

void SnapDataProvider::OnSnapInstalled(
    const mojom::SnapInstallData& install_data,
    base::OnceClosure callback) {
  snap_registry_->RegisterInstalledSnap(install_data);
  PostClosure(std::move(callback));
}

void SnapDataProvider::OnSnapUninstalled(const std::string& snap_id,
                                         base::OnceClosure callback) {
  snap_registry_->UnregisterSnap(snap_id);
  state_cache_.erase(snap_id);
  snap_storage_.AsyncCall(base::IgnoreResult(&SnapStorage::DeleteSnap))
      .WithArgs(snap_id)
      .Then(std::move(callback));
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void SnapDataProvider::PostBoolCallback(BoolCallback callback, bool value) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), value));
}

void SnapDataProvider::PostSnapCallback(SnapCallback callback,
                                        mojom::SnapInstallDataPtr value) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(value)));
}

void SnapDataProvider::PostSnapsCallback(
    SnapsCallback callback,
    std::vector<mojom::SnapInstallDataPtr> value) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(value)));
}

void SnapDataProvider::PostClosure(base::OnceClosure callback) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
                                                          std::move(callback));
}

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
