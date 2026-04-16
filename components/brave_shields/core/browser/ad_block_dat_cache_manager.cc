// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_view_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

namespace {

constexpr char kAdblockCacheDir[] = "adblock_cache";
constexpr char kAdBlockEngine0DATCache[] = "engine0.dat";
constexpr char kAdBlockEngine1DATCache[] = "engine1.dat";

std::pair<std::optional<DATFileDataBuffer>, std::optional<DATFileDataBuffer>>
ReadCachedDATFilesFromDisk(bool has_cached_default_dat,
                           bool has_cached_additional_dat,
                           base::FilePath cache_dir) {
  if (!base::CreateDirectory(cache_dir)) {
    return std::make_pair(std::nullopt, std::nullopt);
  }

  base::FilePath default_engine_dat_file =
      cache_dir.AppendASCII(kAdBlockEngine0DATCache);
  base::FilePath additional_engine_dat_file =
      cache_dir.AppendASCII(kAdBlockEngine1DATCache);

  return std::make_pair(has_cached_default_dat
                            ? base::ReadFileToBytes(default_engine_dat_file)
                            : std::make_optional<DATFileDataBuffer>(),
                        has_cached_additional_dat
                            ? base::ReadFileToBytes(additional_engine_dat_file)
                            : std::make_optional<DATFileDataBuffer>());
}

}  // namespace

// static
void AdBlockDATCacheManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kAdBlockDefaultDATCacheTimestamp, "");
  registry->RegisterStringPref(prefs::kAdBlockAdditionalDATCacheTimestamp, "");
}

AdBlockDATCacheManager::AdBlockDATCacheManager(
    PrefService* local_state,
    const base::FilePath& profile_dir)
    : local_state_(local_state),
      cache_dir_(profile_dir.AppendASCII(kAdblockCacheDir)),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}

AdBlockDATCacheManager::~AdBlockDATCacheManager() = default;

// static
std::string_view AdBlockDATCacheManager::TimestampPrefName(
    bool is_default_engine) {
  return is_default_engine ? prefs::kAdBlockDefaultDATCacheTimestamp
                           : prefs::kAdBlockAdditionalDATCacheTimestamp;
}

bool AdBlockDATCacheManager::HasCachedDAT(bool is_default_engine) const {
  if (!base::FeatureList::IsEnabled(features::kAdblockDATCache)) {
    return false;
  }
  if (!local_state_) {
    CHECK_IS_TEST();
    return false;
  }
  return !local_state_->GetString(TimestampPrefName(is_default_engine)).empty();
}

void AdBlockDATCacheManager::MarkDATCacheWritten(bool is_default_engine) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  if (!local_state_) {
    CHECK_IS_TEST();
    return;
  }
  local_state_->SetString(
      TimestampPrefName(is_default_engine),
      base::NumberToString(base::Time::Now().InMillisecondsFSinceUnixEpoch()));
}

void AdBlockDATCacheManager::WriteDATFile(
    bool is_default_engine,
    DATFileDataBuffer dat,
    base::OnceCallback<void(bool)> on_complete) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](DATFileDataBuffer dat, base::FilePath cache_path) {
            if (!base::CreateDirectory(cache_path.DirName())) {
              return false;
            }
            return base::ImportantFileWriter::WriteFileAtomically(
                cache_path, base::as_string_view(dat));
          },
          std::move(dat),
          cache_dir_.AppendASCII(is_default_engine ? kAdBlockEngine0DATCache
                                                   : kAdBlockEngine1DATCache)),
      base::BindOnce(&AdBlockDATCacheManager::OnDATFileWritten,
                     weak_factory_.GetWeakPtr(), is_default_engine,
                     std::move(on_complete)));
}

void AdBlockDATCacheManager::OnDATFileWritten(
    bool is_default_engine,
    base::OnceCallback<void(bool)> on_complete,
    bool success) {
  if (success) {
    MarkDATCacheWritten(is_default_engine);
  }
  std::move(on_complete).Run(success);
}

void AdBlockDATCacheManager::MaybeReadCachedDATFiles(
    base::OnceCallback<void(std::optional<DATFileDataBuffer>,
                            std::optional<DATFileDataBuffer>)> on_complete) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadCachedDATFilesFromDisk, HasCachedDAT(true),
                     HasCachedDAT(false), cache_dir_),
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
