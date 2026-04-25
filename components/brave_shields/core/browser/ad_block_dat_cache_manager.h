// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_DAT_CACHE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_DAT_CACHE_MANAGER_H_

#include <optional>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

// Manages DAT file caching for adblock engines. Handles reading cached DATs
// on startup and writing serialized engine data to disk.
class AdBlockDATCacheManager {
 public:
  explicit AdBlockDATCacheManager(const base::FilePath& profile_dir);
  ~AdBlockDATCacheManager();
  AdBlockDATCacheManager(const AdBlockDATCacheManager&) = delete;
  AdBlockDATCacheManager& operator=(const AdBlockDATCacheManager&) = delete;

  // Writes a serialized DAT buffer to disk atomically.
  // Calls |on_complete| with success/failure.
  void WriteDATFile(bool is_default_engine, DATFileDataBuffer dat);

  // Reads cached DAT files from disk. Calls |on_complete| with the results.
  void MaybeReadCachedDATFiles(
      base::OnceCallback<void(std::optional<DATFileDataBuffer>,
                              std::optional<DATFileDataBuffer>)> on_complete);

 private:
  base::FilePath cache_dir_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_DAT_CACHE_MANAGER_H_
