/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_STORAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_STORAGE_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"

namespace base {
class SequencedTaskRunner;
}

namespace brave_wallet {

// SnapStorage manages on-disk persistence of snap bundles.
//
// Bundles are stored under |snaps_dir| with one subdirectory per snap
// subdirectory per snap (snap_id characters ':' and '/' replaced by '_'):
//
//   npm_@polkagate_snap/
//     bundle.js       (JS source, up to ~10 MB)
//     manifest.json   (snap.manifest.json, a few KB)
//
// SnapStorage is a synchronous file helper. It is owned by SnapDataProvider via
// base::SequenceBound and every method must run on that background sequence.
class SnapStorage {
 public:
  SnapStorage(const base::FilePath& snaps_dir,
              scoped_refptr<base::SequencedTaskRunner> origin_task_runner);
  ~SnapStorage();

  SnapStorage(const SnapStorage&) = delete;
  SnapStorage& operator=(const SnapStorage&) = delete;

  // Moves bundle.js and manifest.json from |unpacked_dir| to the persistent
  // snap directory for |snap_id|. Returns false on I/O error.
  bool MoveSnapFiles(const std::string& snap_id,
                     const base::FilePath& unpacked_dir);

  // Reads bundle.js for |snap_id| from disk. Returns std::nullopt on error.
  std::optional<std::string> ReadBundle(const std::string& snap_id);

  // Deletes the snap directory and all its contents.
  bool DeleteSnap(const std::string& snap_id);

  // Writes |json| to state.json in the snap's directory. Creates the directory
  // if it does not exist (so this works for built-in snaps that have no bundle
  // on disk). Returns false on I/O error.
  bool WriteState(const std::string& snap_id, const std::string& json);

  // Reads state.json from the snap's directory. Returns std::nullopt if the
  // file does not exist or cannot be read.
  std::optional<std::string> ReadState(const std::string& snap_id);

  // Synchronous check: returns true if bundle.js exists on disk.
  // Only call from a MayBlock context or during initialisation.
  bool HasSnap(const std::string& snap_id) const;

 private:
  // Returns the snap-specific subdirectory path for |snap_id|.
  base::FilePath GetSnapDir(const std::string& snap_id) const;

  // Root directory passed to the constructor (typically
  // <profile>/BraveWallet/Snaps/).
  base::FilePath snaps_dir_;

  scoped_refptr<base::SequencedTaskRunner> origin_task_runner_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_STORAGE_H_
