// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
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
ReadCachedDATFilesFromDisk(base::FilePath cache_dir) {
  if (!base::CreateDirectory(cache_dir)) {
    return std::make_pair(std::nullopt, std::nullopt);
  }

  base::FilePath default_engine_dat_file =
      cache_dir.AppendASCII(kAdBlockEngine0DATCache);
  base::FilePath additional_engine_dat_file =
      cache_dir.AppendASCII(kAdBlockEngine1DATCache);

  return std::make_pair(base::ReadFileToBytes(default_engine_dat_file),
                        base::ReadFileToBytes(additional_engine_dat_file));
}

}  // namespace

// static
void AdBlockDATCacheManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kAdBlockDefaultCacheHash, "");
  registry->RegisterStringPref(prefs::kAdBlockAdditionalCacheHash, "");
}

AdBlockDATCacheManager::AdBlockDATCacheManager(
    PrefService* local_state,
    AdBlockFiltersProviderManager* provider_manager,
    const base::FilePath& profile_dir)
    : local_state_(local_state),
      provider_manager_(provider_manager),
      cache_dir_(profile_dir.AppendASCII(kAdblockCacheDir)),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  if (base::FeatureList::IsEnabled(features::kAdblockDATCache) &&
      local_state_ &&
      (!local_state_->GetString(CacheHashPrefName(true)).empty() ||
       !local_state_->GetString(CacheHashPrefName(false)).empty())) {
    provider_manager_->SetWaitForComponentProviders(true);
  }
}

AdBlockDATCacheManager::~AdBlockDATCacheManager() = default;

std::string AdBlockDATCacheManager::ComputeCombinedCacheKey(
    bool is_default_engine) const {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  std::vector<std::string> keys;
  for (auto* provider : provider_manager_->GetProviders(is_default_engine)) {
    auto key = provider->GetCacheKey();
    if (key.has_value()) {
      keys.push_back(std::move(*key));
    }
  }
  std::sort(keys.begin(), keys.end());
  return base::JoinString(keys, "|");
}

bool AdBlockDATCacheManager::ShouldLoadFilterSet(bool is_default_engine) const {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  if (!local_state_) {
    CHECK_IS_TEST();
    return true;
  }
  std::string combined_key = ComputeCombinedCacheKey(is_default_engine);
  std::string cached_key =
      local_state_->GetString(CacheHashPrefName(is_default_engine));
  return cached_key.empty() || cached_key != combined_key;
}

void AdBlockDATCacheManager::StoreCacheKey(bool is_default_engine,
                                           const std::string& cache_key) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  if (!local_state_) {
    CHECK_IS_TEST();
  } else {
    local_state_->SetString(CacheHashPrefName(is_default_engine), cache_key);
  }
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
      std::move(on_complete));
}

void AdBlockDATCacheManager::ReadCachedDATFiles(
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

// static
std::string_view AdBlockDATCacheManager::CacheHashPrefName(
    bool is_default_engine) {
  return is_default_engine ? prefs::kAdBlockDefaultCacheHash
                           : prefs::kAdBlockAdditionalCacheHash;
}

bool AdBlockDATCacheManager::allow_dat_loading() const {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  return allow_dat_loading_;
}

void AdBlockDATCacheManager::set_allow_dat_loading(bool allow) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  allow_dat_loading_ = allow;
}

}  // namespace brave_shields
