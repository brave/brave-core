/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_DATA_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_DATA_PROVIDER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class PrefService;

namespace brave_wallet {

class SnapStorage;

// SnapDataProvider is the facade for all installed-snap data.
// It owns SnapRegistry (in-memory manifest catalogue), SnapStorage (on-disk
// bundle + state files), and the in-session state cache. Callers interact
// exclusively through this class's methods — the inner objects are never
// exposed directly.
class SnapDataProvider {
 public:
  SnapDataProvider(const base::FilePath& profile_path, PrefService& prefs);
  ~SnapDataProvider();

  SnapDataProvider(const SnapDataProvider&) = delete;
  SnapDataProvider& operator=(const SnapDataProvider&) = delete;

  // ---------------------------------------------------------------------------
  // Registry queries (in-memory, synchronous)
  // ---------------------------------------------------------------------------

  bool IsInstalled(const std::string& snap_id) const;
  // Returns true when |snap_id| is installed and its enabled flag is set.
  bool IsSnapEnabled(const std::string& snap_id) const;
  // Returns a clone of the snap's install data, or nullptr if not installed.
  mojom::SnapInstallDataPtr GetSnap(const std::string& snap_id) const;
  std::vector<mojom::SnapInstallDataPtr> GetAllSnaps() const;
  void SetSnapEnabled(const std::string& snap_id, bool enabled);

  // ---------------------------------------------------------------------------
  // Bundle I/O (async, thread-pool)
  // ---------------------------------------------------------------------------

  // Moves bundle.js and manifest.json from |unpacked_dir| (a subfolder of the
  // snap's install-time ScopedTempDir) to the persistent snap directory.
  void SaveBundleFromDir(const std::string& snap_id,
                         const base::FilePath& unpacked_dir,
                         base::OnceCallback<void(bool)> on_done);

  void ReadBundle(const std::string& snap_id,
                  base::OnceCallback<void(std::optional<std::string>)> cb);

  // ---------------------------------------------------------------------------
  // Snap state — unified cache + storage (async for reads, sync cache update)
  // ---------------------------------------------------------------------------

  using StateCallback =
      base::OnceCallback<void(std::optional<std::string> json)>;
  // Callback for write operations. |error| is nullopt on success, or a
  // human-readable message on validation or I/O failure.
  using StateWriteCallback =
      base::OnceCallback<void(std::optional<std::string> error)>;

  // Returns the cached state if already loaded this session; otherwise reads
  // state.json from disk, caches the result, then invokes |callback| with the
  // raw JSON string (or nullopt if no state exists).
  void GetSnapState(const std::string& snap_id, StateCallback callback);

  // Validates |json| against the 1 MB size limit. On failure |on_done| is
  // called synchronously with an error and no state is modified. On success
  // the cache is updated immediately, the disk write starts asynchronously, and
  // |on_done| is invoked when the disk write callback runs.
  void UpdateSnapState(const std::string& snap_id,
                       std::string json,
                       StateWriteCallback on_done);

  // Clears the in-memory cache and writes "{}" to disk asynchronously.
  // |on_done| is invoked when the disk write callback runs.
  void ClearSnapState(const std::string& snap_id, StateWriteCallback on_done);

  // ---------------------------------------------------------------------------
  // Install lifecycle — called by SnapInstaller after pref writes
  // ---------------------------------------------------------------------------

  // Writes snap metadata to PrefService, updates the in-memory registry, and
  // records the install timestamp. Called by SnapInstaller after the bundle
  // file has been successfully saved to disk.
  void OnSnapInstalled(const mojom::SnapInstallData& install_data);

  // Removes snap metadata from PrefService, clears the in-memory registry
  // entry, evicts the state cache, and deletes the bundle directory.
  void OnSnapUninstalled(const std::string& snap_id);

 private:
  void OnStateLoaded(std::string snap_id,
                     StateCallback callback,
                     std::optional<std::string> disk_json);
  static void OnStatePersisted(StateWriteCallback on_done, bool success);
  static void OnStateCleared(StateWriteCallback on_done, bool success);

  std::unique_ptr<SnapStorage> snap_storage_;
  std::unique_ptr<SnapRegistry> snap_registry_;

  // snap_id → raw JSON string (same bytes as state.json on disk).
  // Absent = not yet loaded from disk this session.
  // Present, empty string = no state (cleared or never written).
  std::map<std::string, std::string> state_cache_;

  base::WeakPtrFactory<SnapDataProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_DATA_PROVIDER_H_
