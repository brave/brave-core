// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_service.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/debug/leak_annotations.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_localhost_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/features.h"

namespace brave_shields {

namespace {
constexpr char kAdblockCacheDir[] = "adblock_cache";
constexpr char kAdBlockEngine0DATCache[] = "engine0.dat";
constexpr char kAdBlockEngine1DATCache[] = "engine1.dat";

std::pair<std::optional<DATFileDataBuffer>, std::optional<DATFileDataBuffer>>
ReadCachedDATFiles(base::FilePath cache_dir) {
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

AdBlockService::SourceProviderObserver::SourceProviderObserver(
    OnResourcesLoadedCallback on_resources_loaded,
    AdBlockResourceProvider* resource_provider,
    AdBlockFiltersProviderManager* filters_provider_manager,
    ShouldLoadFilterSetCallback should_load_filter_set,
    bool engine_is_default,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : on_resources_loaded_(std::move(on_resources_loaded)),
      should_load_filter_set_(should_load_filter_set),
      engine_is_default_(engine_is_default),
      task_runner_(std::move(task_runner)),
      resource_provider_(resource_provider),
      filters_provider_manager_(filters_provider_manager) {
  filters_provider_manager_->AddObserver(this);
  filters_provider_manager_->ForceNotifyObserver(*this, engine_is_default_);
}

AdBlockService::SourceProviderObserver::~SourceProviderObserver() {
  filters_provider_manager_->RemoveObserver(this);
  resource_provider_->RemoveObserver(this);
}

void AdBlockService::SourceProviderObserver::OnChanged(bool is_default_engine) {
  if (engine_is_default_ != is_default_engine) {
    // Skip updates of another engine.
    return;
  }

  if (!should_load_filter_set_.Run()) {
    // Skip updates that have already been cached.
    return;
  }
  auto on_loaded_cb =
      base::BindOnce(&AdBlockService::SourceProviderObserver::OnFilterSetLoaded,
                     weak_factory_.GetWeakPtr());
  filters_provider_manager_->LoadFilterSetForEngine(is_default_engine,
                                                    std::move(on_loaded_cb));
}

void AdBlockService::SourceProviderObserver::OnFilterSetLoaded(
    base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> cb) {
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> cb) {
            auto filter_set = std::make_unique<rust::Box<adblock::FilterSet>>(
                adblock::new_filter_set());
            std::move(cb).Run(filter_set.get());
            return filter_set;
          },
          std::move(cb)),
      base::BindOnce(
          &AdBlockService::SourceProviderObserver::OnFilterSetCreated,
          weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::LoadResources() {
  // multiple AddObserver calls are ignored
  resource_provider_->AddObserver(this);
  resource_provider_->LoadResources(base::BindOnce(
      &SourceProviderObserver::OnResourcesLoaded, weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::OnDATFileLoaded(
    DATFileDataBuffer dat) {
  dat_.emplace(std::move(dat));
  LoadResources();
}

void AdBlockService::SourceProviderObserver::OnFilterSetCreated(
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set) {
  TRACE_EVENT("brave.adblock", "OnFilterSetCreated");
  filter_set_ = std::move(filter_set);
  LoadResources();
}

void AdBlockService::SourceProviderObserver::OnResourcesLoaded(
    AdblockResourceStorageBox storage) {
  on_resources_loaded_.Run(
      engine_is_default_, std::exchange(dat_, std::nullopt),
      std::exchange(filter_set_, nullptr), std::move(storage));
}

AdBlockComponentServiceManager* AdBlockService::component_service_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return component_service_manager_.get();
}

AdBlockCustomFiltersProvider* AdBlockService::custom_filters_provider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return custom_filters_provider_.get();
}

AdBlockCustomResourceProvider* AdBlockService::custom_resource_provider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return custom_resource_provider_.get();
}

AdBlockSubscriptionServiceManager*
AdBlockService::subscription_service_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return subscription_service_manager_.get();
}

AdBlockService::AdBlockService(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    AdBlockSubscriptionDownloadManager::DownloadManagerGetter
        subscription_download_manager_getter,
    const base::FilePath& profile_dir)
    : local_state_(local_state),
      locale_(locale),
      profile_dir_(profile_dir),
      subscription_download_manager_getter_(
          std::move(subscription_download_manager_getter)),
      component_update_service_(cus),
      task_runner_(task_runner),
      list_p3a_(local_state),
      engine_wrapper_(AdBlockEngineWrapper::Create().release(),
                      base::OnTaskRunnerDeleter(task_runner_.get())) {
  TRACE_EVENT("brave.adblock", "AdBlockService");
  // Initializes adblock-rust's domain resolution implementation
  adblock::set_domain_resolver();

  if (base::FeatureList::IsEnabled(
          features::kAdblockOverrideRegexDiscardPolicy)) {
    adblock::RegexManagerDiscardPolicy policy;
    policy.cleanup_interval_secs =
        features::kAdblockOverrideRegexDiscardPolicyCleanupIntervalSec.Get();
    policy.discard_unused_secs =
        features::kAdblockOverrideRegexDiscardPolicyDiscardUnusedSec.Get();
    SetupDiscardPolicy(policy);
  }

  auto default_resource_provider =
      std::make_unique<AdBlockDefaultResourceProvider>(
          component_update_service_);
  default_resource_provider_ = default_resource_provider.get();

  custom_resource_provider_ = new AdBlockCustomResourceProvider(
      profile_dir_, std::move(default_resource_provider));
  resource_provider_.reset(custom_resource_provider_.get());

  if (base::FeatureList::IsEnabled(features::kAdblockDATCache)) {
    base::FilePath cache_dir = profile_dir_.AppendASCII(kAdblockCacheDir);
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
        base::BindOnce(&ReadCachedDATFiles, cache_dir),
        base::BindOnce(&AdBlockService::OnReadCachedDATFiles,
                       weak_factory_.GetWeakPtr()));
  }

  filter_list_catalog_provider_ =
      std::make_unique<AdBlockFilterListCatalogProvider>(
          component_update_service_);

  filters_provider_manager_ = std::make_unique<AdBlockFiltersProviderManager>();

  component_service_manager_ = std::make_unique<AdBlockComponentServiceManager>(
      local_state_, filters_provider_manager_.get(), locale_,
      component_update_service_, filter_list_catalog_provider_.get(),
      &list_p3a_);
  subscription_service_manager_ =
      std::make_unique<AdBlockSubscriptionServiceManager>(
          local_state_, filters_provider_manager_.get(),
          std::move(subscription_download_manager_getter_), profile_dir_,
          &list_p3a_);
  custom_filters_provider_ = std::make_unique<AdBlockCustomFiltersProvider>(
      local_state_, filters_provider_manager_.get());

  if (base::FeatureList::IsEnabled(
          network::features::kLocalNetworkAccessChecks) &&
      !network::features::kLocalNetworkAccessChecksWarn.Get() &&
      base::FeatureList::IsEnabled(
          network::features::kLocalNetworkAccessChecksWebSockets)) {
    // If LNA enabled and blocks request
    localhost_filters_provider_ =
        std::make_unique<AdBlockLocalhostFiltersProvider>(
            filters_provider_manager_.get());
  }

  const auto make_on_resources_loaded_callback = base::BindRepeating(
      &AdBlockService::OnResourcesLoaded, base::Unretained(this));

  const auto should_load_filter_state_callback = base::BindRepeating(
      &AdBlockService::ShouldLoadFilterState, base::Unretained(this));

  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      make_on_resources_loaded_callback, resource_provider_.get(),
      filters_provider_manager_.get(),
      base::BindRepeating(should_load_filter_state_callback, true), true,
      task_runner_);
  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          make_on_resources_loaded_callback, resource_provider_.get(),
          filters_provider_manager_.get(),
          base::BindRepeating(should_load_filter_state_callback, false), false,
          task_runner_);
}

AdBlockService::~AdBlockService() {
  // The engines are deleted on the task runner with SKIP_ON_SHUTDOWN trait,
  // therefore they leak during shutdown.
  ANNOTATE_LEAKING_OBJECT_PTR(engine_wrapper_.get());
}

void AdBlockService::OnResourcesLoaded(
    bool is_default_engine,
    std::optional<DATFileDataBuffer> dat,
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
    AdblockResourceStorageBox storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (dat.has_value()) {
    if (allow_load_dat_loading_) {
      task_runner_->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&AdBlockEngineWrapper::LoadDAT,
                         base::Unretained(engine_wrapper_.get()),
                         is_default_engine, std::move(*dat),
                         std::move(storage)),
          base::BindOnce(&AdBlockService::NotifyOnDATLoaded,
                         weak_factory_.GetWeakPtr(), is_default_engine));
    } else {
      NotifyOnDATLoaded(is_default_engine, false);
    }
  } else if (base::FeatureList::IsEnabled(features::kAdblockDATCache)) {
    // Load the filter set on the engine's task runner, then serialize and
    // write the DAT cache on a thread pool to avoid blocking the engine.
    task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(
            [](AdBlockEngineWrapper* engine_wrapper, bool is_default_engine,
               std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
               AdblockResourceStorageBox storage) -> DATFileDataBuffer {
              if (!engine_wrapper->Load(is_default_engine,
                                        std::move(filter_set),
                                        std::move(storage))) {
                return {};
              }
              return engine_wrapper->Serialize(is_default_engine);
            },
            base::Unretained(engine_wrapper_.get()), is_default_engine,
            std::move(filter_set), std::move(storage)),
        base::BindOnce(&AdBlockService::OnEngineLoaded,
                       weak_factory_.GetWeakPtr(), is_default_engine));
    // Block DAT loading if a FilterList has been loaded already
    allow_load_dat_loading_ = false;
  } else {
    task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&AdBlockEngineWrapper::Load,
                       base::Unretained(engine_wrapper_.get()),
                       is_default_engine, std::move(filter_set),
                       std::move(storage)),
        base::BindOnce(&AdBlockService::OnDatCached, weak_factory_.GetWeakPtr(),
                       is_default_engine));
  }

  for (auto& observer : observers_) {
    observer.OnResourcesLoaded(is_default_engine);
  }
}

void AdBlockService::NotifyOnDATLoaded(bool is_default_engine, bool success) {
  for (auto& observer : observers_) {
    observer.OnDATFileLoaded(is_default_engine, success);
  }
}

void AdBlockService::OnEngineLoaded(bool is_default_engine,
                                    DATFileDataBuffer serialized_dat) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (serialized_dat.empty()) {
    OnDatCached(is_default_engine, false);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(
          [](DATFileDataBuffer dat, base::FilePath cache_path) {
            if (!base::CreateDirectory(cache_path.DirName())) {
              return false;
            }
            return base::ImportantFileWriter::WriteFileAtomically(
                cache_path, base::as_string_view(dat));
          },
          std::move(serialized_dat),
          profile_dir_.AppendASCII(kAdblockCacheDir)
              .AppendASCII(is_default_engine ? kAdBlockEngine0DATCache
                                             : kAdBlockEngine1DATCache)),
      base::BindOnce(&AdBlockService::OnDatCached, weak_factory_.GetWeakPtr(),
                     is_default_engine));
}

void AdBlockService::OnDatCached(bool is_default_engine, bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (success) {
    std::string combined_key = ComputeCombinedCacheKey(is_default_engine);
    if (!combined_key.empty()) {
      if (!local_state_) {
        CHECK_IS_TEST();
      } else {
        local_state_->SetString(cache_hash_pref_name(is_default_engine),
                                combined_key);
      }
    }
  }
  for (auto& observer : observers_) {
    observer.OnFilterListLoaded(is_default_engine, success);
  }
}

void AdBlockService::OnReadCachedDATFiles(
    std::pair<std::optional<DATFileDataBuffer>,
              std::optional<DATFileDataBuffer>> read_result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (read_result.first && allow_load_dat_loading_) {
    default_service_observer_->OnDATFileLoaded(std::move(*read_result.first));
    NotifyOnDATLoaded(true, true);
  } else {
    NotifyOnDATLoaded(true, false);
  }

  if (read_result.second && allow_load_dat_loading_) {
    additional_filters_service_observer_->OnDATFileLoaded(
        std::move(*read_result.second));
    NotifyOnDATLoaded(false, true);
  } else {
    NotifyOnDATLoaded(false, false);
  }
}

void AdBlockService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AdBlockService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdBlockService::EnableTag(const std::string& tag, bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Tags only need to be modified for the default engine.
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngineWrapper::EnableTag,
                     base::Unretained(engine_wrapper_.get()), tag, enabled));
}

void AdBlockService::AddUserCosmeticFilter(const std::string& filter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  custom_filters_provider_->AddUserCosmeticFilter(filter);
}

bool AdBlockService::AreAnyBlockedElementsPresent(std::string_view host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return custom_filters_provider_->AreAnyBlockedElementsPresent(host);
}

void AdBlockService::ResetCosmeticFilter(std::string_view host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  custom_filters_provider_->ResetCosmeticFilter(host);
}

void AdBlockService::GetDebugInfoAsync(GetDebugInfoCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockEngineWrapper::GetDebugInfo,
                     base::Unretained(engine_wrapper_.get())),
      std::move(callback));
}

void AdBlockService::DiscardRegex(uint64_t regex_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngineWrapper::DiscardRegex,
                     base::Unretained(engine_wrapper_.get()), regex_id));
}

void AdBlockService::SetupDiscardPolicy(
    const adblock::RegexManagerDiscardPolicy& policy) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngineWrapper::SetupDiscardPolicy,
                     base::Unretained(engine_wrapper_.get()), policy));
}

std::string AdBlockService::ComputeCombinedCacheKey(
    bool is_default_engine) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<std::string> keys;
  for (auto* provider :
       filters_provider_manager_->GetProviders(is_default_engine)) {
    auto key = provider->GetCacheKey();
    if (key.has_value()) {
      keys.push_back(std::move(*key));
    }
  }
  std::sort(keys.begin(), keys.end());
  return base::JoinString(keys, "|");
}

bool AdBlockService::ShouldLoadFilterState(bool is_default_engine) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::FeatureList::IsEnabled(features::kAdblockDATCache)) {
    return true;
  }
  if (!local_state_) {
    CHECK_IS_TEST();
    return true;
  }
  std::string combined_key = ComputeCombinedCacheKey(is_default_engine);
  std::string cached_key =
      local_state_->GetString(cache_hash_pref_name(is_default_engine));
  return cached_key.empty() || cached_key != combined_key;
}

std::string_view AdBlockService::cache_hash_pref_name(bool engine_is_default) {
  return engine_is_default ? prefs::kAdBlockDefaultCacheHash
                           : prefs::kAdBlockAdditionalCacheHash;
}

void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockCookieListSettingTouched, false);
  registry->RegisterBooleanPref(
      prefs::kAdBlockMobileNotificationsListSettingTouched, false);
  registry->RegisterStringPref(prefs::kAdBlockCustomFilters, std::string());
  registry->RegisterDictionaryPref(prefs::kAdBlockRegionalFilters);
  registry->RegisterDictionaryPref(prefs::kAdBlockListSubscriptions);
  registry->RegisterBooleanPref(prefs::kAdBlockCheckedDefaultRegion, false);
  registry->RegisterBooleanPref(prefs::kAdBlockCheckedAllDefaultRegions, false);
  registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);
  registry->RegisterBooleanPref(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, false);
  registry->RegisterStringPref(prefs::kAdBlockDefaultCacheHash, "");
  registry->RegisterStringPref(prefs::kAdBlockAdditionalCacheHash, "");
  registry->RegisterDictionaryPref(prefs::kAdBlockSubscriptionFiltersCacheHash);
}

void RegisterPrefsForAdBlockServiceForMigration(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockCookieListOptInShown, false);
}

void MigrateObsoletePrefsForAdBlockService(PrefService* local_state) {
  // Added 2025-07-11
  local_state->ClearPref(prefs::kAdBlockCookieListOptInShown);
}

AdBlockDefaultResourceProvider* AdBlockService::default_resource_provider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return default_resource_provider_.get();
}

AdBlockEngine& AdBlockService::GetDefaultEngineForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return engine_wrapper_->default_engine_for_testing();  // IN-TEST
}

AdBlockEngine& AdBlockService::GetAdditionalFiltersEngineForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return engine_wrapper_->additional_filters_engine_for_testing();  // IN-TEST
}

AdBlockFiltersProviderManager*
AdBlockService::GetFiltersProviderManagerForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return filters_provider_manager_.get();
}

AdBlockDefaultResourceProvider*
AdBlockService::GetDefaultResourceProviderForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return default_resource_provider_.get();
}

base::SequencedTaskRunner* AdBlockService::GetTaskRunnerForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return task_runner_.get();
}

bool AdBlockService::GetAllowDatLoadingForTesting() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return allow_load_dat_loading_;
}

std::string AdBlockService::ComputeCombinedCacheKeyForTesting(
    bool is_default_engine) const {
  CHECK_IS_TEST();
  return ComputeCombinedCacheKey(is_default_engine);
}

}  // namespace brave_shields
