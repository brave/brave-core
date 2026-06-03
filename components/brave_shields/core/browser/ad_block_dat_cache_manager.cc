// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_view_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

namespace {

constexpr char kAdblockCacheDir[] = "adblock_cache";
constexpr char kAdBlockEngine0DATCache[] = "engine0.dat";
constexpr char kAdBlockEngine1DATCache[] = "engine1.dat";

std::pair<std::optional<DATFileDataBuffer>, std::optional<DATFileDataBuffer>>
ReadCachedDATFilesFromDisk(base::FilePath cache_dir) {
  base::FilePath default_engine_dat_file =
      cache_dir.AppendASCII(kAdBlockEngine0DATCache);
  base::FilePath additional_engine_dat_file =
      cache_dir.AppendASCII(kAdBlockEngine1DATCache);

  return std::make_pair(base::ReadFileToBytes(default_engine_dat_file),
                        base::ReadFileToBytes(additional_engine_dat_file));
}

}  // namespace

AdBlockDATCacheManager::AdBlockDATCacheManager(
    const base::FilePath& profile_dir)
    : cache_dir_(profile_dir.AppendASCII(kAdblockCacheDir)),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}

AdBlockDATCacheManager::~AdBlockDATCacheManager() = default;

void AdBlockDATCacheManager::WriteDATFile(bool is_default_engine,
                                          DATFileDataBuffer dat) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](DATFileDataBuffer dat, base::FilePath cache_path) {
            if (!base::CreateDirectory(cache_path.DirName())) {
              return;
            }
            base::ImportantFileWriter::WriteFileAtomically(
                cache_path, base::as_string_view(dat));
          },
          std::move(dat),
          cache_dir_.AppendASCII(is_default_engine ? kAdBlockEngine0DATCache
                                                   : kAdBlockEngine1DATCache)));
}

void AdBlockDATCacheManager::MaybeReadCachedDATFiles(
    base::OnceCallback<void(std::optional<DATFileDataBuffer>,
                            std::optional<DATFileDataBuffer>)> on_complete) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ReadCachedDATFilesFromDisk, cache_dir_),
      base::BindOnce(
          [](base::OnceCallback<void(std::optional<DATFileDataBuffer>,
                                     std::optional<DATFileDataBuffer>)>
                 callback,
             std::pair<std::optional<DATFileDataBuffer>,
                       std::optional<DATFileDataBuffer>> result) {
            std::move(callback).Run(std::move(result.first),
                                    std::move(result.second));
          },
          std::move(on_complete)));
}

}  // namespace brave_shields
