/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_STORAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_STORAGE_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"

namespace brave_wallet {

// SnapStorage manages on-disk persistence of snap bundles.
//
// Bundles are stored under <profile>/BraveWallet/Snaps/ with one
// subdirectory per snap (snap_id characters ':' and '/' replaced by '_'):
//
//   npm_@polkagate_snap/
//     bundle.js       (JS source, up to ~10 MB)
//     manifest.json   (snap.manifest.json, a few KB)
//
// All file I/O runs on a base::ThreadPool worker with base::MayBlock().
// Callbacks are dispatched back to the sequence that created SnapStorage.
class SnapStorage {
 public:
  explicit SnapStorage(const base::FilePath& profile_path);
  ~SnapStorage();

  SnapStorage(const SnapStorage&) = delete;
  SnapStorage& operator=(const SnapStorage&) = delete;

  // Writes bundle.js and manifest.json for |snap_id| to disk.
  // |on_done| is called on the originating sequence; argument is false on I/O
  // error.
  void SaveSnap(const std::string& snap_id,
                const std::string& bundle_js,
                const std::string& manifest_json,
                base::OnceCallback<void(bool success)> on_done);

  // Reads bundle.js for |snap_id| from disk. |cb| receives std::nullopt on
  // error.
  void ReadBundle(const std::string& snap_id,
                  base::OnceCallback<void(std::optional<std::string>)> cb);

  // Deletes the snap directory and all its contents. Fire-and-forget.
  void DeleteSnap(const std::string& snap_id);

  // Writes |json| to state.json in the snap's directory. Creates the directory
  // if it does not exist (so this works for built-in snaps that have no bundle
  // on disk). |on_done| receives false on I/O error.
  void WriteState(const std::string& snap_id,
                  const std::string& json,
                  base::OnceCallback<void(bool)> on_done);

  // Reads state.json from the snap's directory. |cb| receives std::nullopt if
  // the file does not exist or cannot be read.
  void ReadState(const std::string& snap_id,
                 base::OnceCallback<void(std::optional<std::string>)> cb);

  // Synchronous check: returns true if bundle.js exists on disk.
  // Only call from a MayBlock context or during initialisation.
  bool HasSnap(const std::string& snap_id) const;

 private:
  // Returns the snap-specific subdirectory path for |snap_id|.
  base::FilePath GetSnapDir(const std::string& snap_id) const;

  // Root directory: <profile>/BraveWallet/Snaps/
  base::FilePath snaps_dir_;

  base::WeakPtrFactory<SnapStorage> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_STORAGE_H_
