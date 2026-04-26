// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_localhost_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"
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

AdBlockService::SourceProviderObserver::SourceProviderObserver(
    OnResourcesLoadedCallback on_resources_loaded,
    AdBlockResourceProvider* resource_provider,
    AdBlockFiltersProviderManager* filters_provider_manager,
    bool engine_is_default,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : on_resources_loaded_(std::move(on_resources_loaded)),
      engine_is_default_(engine_is_default),
      task_runner_(std::move(task_runner)),
      resource_provider_(resource_provider),
      filters_provider_manager_(filters_provider_manager) {
  filters_provider_manager_->AddObserver(this);
  filters_provider_manager_->MaybeNotifyObserver(*this, engine_is_default_);
}

AdBlockService::SourceProviderObserver::~SourceProviderObserver() {
  filters_provider_manager_->RemoveObserver(this);
  resource_provider_->RemoveObserver(this);
}

void AdBlockService::SourceProviderObserver::OnChanged(bool is_default_engine) {
  if (engine_is_default_ != is_default_engine) {
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

void AdBlockService::SourceProviderObserver::LoadResources(
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set) {
  // multiple AddObserver calls are ignored
  resource_provider_->AddObserver(this);
  resource_provider_->LoadResources(
      base::BindOnce(&SourceProviderObserver::OnAllLoaded,
                     weak_factory_.GetWeakPtr(), std::move(filter_set)));
}

void AdBlockService::SourceProviderObserver::OnDATFileRead(
    DATFileDataBuffer dat) {
  // Load the cached DAT immediately with empty resources so filter rules are
  // available for network blocking without waiting for the resource component.
  on_resources_loaded_.Run(engine_is_default_, std::move(dat), nullptr,
                           adblock::new_empty_resource_storage());
  // Kick off resource loading separately — when resources arrive,
  // OnAllLoaded will call UseResources to update them.
  LoadResources(nullptr);
}

void AdBlockService::SourceProviderObserver::OnFilterSetCreated(
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set) {
  TRACE_EVENT("brave.adblock", "OnFilterSetCreated");
  LoadResources(std::move(filter_set));
}

void AdBlockService::SourceProviderObserver::OnResourcesLoaded(
    AdblockResourceStorageBox storage) {
  on_resources_loaded_.Run(engine_is_default_, std::nullopt, nullptr,
                           std::move(storage));
}

void AdBlockService::SourceProviderObserver::OnAllLoaded(
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
    AdblockResourceStorageBox storage) {
  on_resources_loaded_.Run(engine_is_default_, std::nullopt,
                           std::move(filter_set), std::move(storage));
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
      engine_wrapper_(task_runner_,
                      std::make_unique<AdBlockEngine>(true),
                      std::make_unique<AdBlockEngine>(false)) {
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
  filter_list_catalog_provider_ =
      std::make_unique<AdBlockFilterListCatalogProvider>(
          component_update_service_);

  dat_cache_manager_ = std::make_unique<AdBlockDATCacheManager>(profile_dir_);
  // Start reading cached DAT files from disk as early as possible so the
  // engine can be populated before components arrive from the network.
  if (base::FeatureList::IsEnabled(features::kAdblockDATCache)) {
    dat_cache_manager_->MaybeReadCachedDATFiles(base::BindOnce(
        &AdBlockService::OnReadCachedDATFiles, weak_factory_.GetWeakPtr()));
  }

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

  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      make_on_resources_loaded_callback, resource_provider_.get(),
      filters_provider_manager_.get(), true, task_runner_);
  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          make_on_resources_loaded_callback, resource_provider_.get(),
          filters_provider_manager_.get(), false, task_runner_);
}

AdBlockService::~AdBlockService() = default;

void AdBlockService::OnResourcesLoaded(
    bool is_default_engine,
    std::optional<DATFileDataBuffer> dat,
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
    AdblockResourceStorageBox storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (dat.has_value()) {
    CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
    AsyncCallAndReplyWithResult(
        base::BindOnce(
            [](bool is_default, std::optional<DATFileDataBuffer> dat,
               AdblockResourceStorageBox storage,
               AdBlockEngineWrapper* wrapper) {
              return wrapper->LoadDAT(is_default, std::move(*dat),
                                      std::move(storage));
            },
            is_default_engine, std::move(*dat), std::move(storage)),
        base::BindOnce(&AdBlockService::OnDATLoaded, weak_factory_.GetWeakPtr(),
                       is_default_engine));
  } else {
    bool should_cache =
        base::FeatureList::IsEnabled(features::kAdblockDATCache);
    AsyncCallAndReplyWithResult(
        base::BindOnce(
            [](bool is_default, bool cache,
               std::unique_ptr<rust::Box<adblock::FilterSet>> fs,
               AdblockResourceStorageBox s, AdBlockEngineWrapper* wrapper)
                -> std::pair<FilterListLoadResult,
                             std::optional<DATFileDataBuffer>> {
              bool resources_only = !fs;
              if (!wrapper->Load(is_default, std::move(fs), std::move(s))) {
                return {FilterListLoadResult::kFailed, std::nullopt};
              }
              if (resources_only) {
                return {FilterListLoadResult::kResourcesOnly, std::nullopt};
              }
              if (!cache) {
                return {FilterListLoadResult::kLoaded, std::nullopt};
              }
              return {FilterListLoadResult::kLoaded,
                      wrapper->Serialize(is_default)};
            },
            is_default_engine, should_cache, std::move(filter_set),
            std::move(storage)),
        base::BindOnce(&AdBlockService::OnEngineLoaded,
                       weak_factory_.GetWeakPtr(), is_default_engine));
  }
}

void AdBlockService::OnDATLoaded(bool is_default_engine, bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (is_default_engine) {
    default_dat_loaded_for_testing_ = true;
  } else {
    additional_dat_loaded_for_testing_ = true;
  }
  // If the cached DAT failed to load, fall back to loading from filter lists.
  if (!success) {
    if (is_default_engine) {
      filters_provider_manager_->ForceNotifyObserver(*default_service_observer_,
                                                     true);
    } else {
      filters_provider_manager_->ForceNotifyObserver(
          *additional_filters_service_observer_, false);
    }
  }
  observers_.Notify(&Observer::OnDATLoaded, is_default_engine, success);
}

void AdBlockService::OnEngineLoaded(
    bool is_default_engine,
    std::pair<FilterListLoadResult, std::optional<DATFileDataBuffer>> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto [load_result, serialized_dat] = std::move(result);

  if (load_result != FilterListLoadResult::kResourcesOnly) {
    if (is_default_engine) {
      default_filter_list_loaded_for_testing_ = true;
    } else {
      additional_filter_list_loaded_for_testing_ = true;
    }
    observers_.Notify(&Observer::OnFilterListLoaded, is_default_engine,
                      load_result);
  }

  if (serialized_dat.has_value() && !serialized_dat->empty()) {
    dat_cache_manager_->WriteDATFile(is_default_engine,
                                     std::move(*serialized_dat));
  }
}

void AdBlockService::OnReadCachedDATFiles(
    std::optional<DATFileDataBuffer> default_dat,
    std::optional<DATFileDataBuffer> additional_dat) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Load cached DATs unconditionally for immediate ad-blocking protection.
  // If a DAT file is missing or fails to load, OnDATLoaded will
  // fall back to loading from filter lists via ForceNotifyObserver.
  if (default_dat) {
    default_service_observer_->OnDATFileRead(std::move(*default_dat));
  } else {
    OnDATLoaded(true, false);
  }

  if (additional_dat) {
    additional_filters_service_observer_->OnDATFileRead(
        std::move(*additional_dat));
  } else {
    OnDATLoaded(false, false);
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
  engine_wrapper_.AsyncCall(&AdBlockEngineWrapper::EnableTag)
      .WithArgs(tag, enabled);
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
  engine_wrapper_.AsyncCall(&AdBlockEngineWrapper::GetDebugInfo)
      .Then(std::move(callback));
}

void AdBlockService::DiscardRegex(uint64_t regex_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  engine_wrapper_.AsyncCall(&AdBlockEngineWrapper::DiscardRegex)
      .WithArgs(regex_id);
}

void AdBlockService::SetupDiscardPolicy(
    const adblock::RegexManagerDiscardPolicy& policy) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  engine_wrapper_.AsyncCall(&AdBlockEngineWrapper::SetupDiscardPolicy)
      .WithArgs(policy);
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

AdBlockDATCacheManager* AdBlockService::GetDATCacheManagerForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_IS_TEST();
  return dat_cache_manager_.get();
}

bool AdBlockService::IsDATLoadedForTesting(bool is_default_engine) const {
  CHECK_IS_TEST();
  return is_default_engine ? default_dat_loaded_for_testing_
                           : additional_dat_loaded_for_testing_;
}

bool AdBlockService::IsFilterListLoadedForTesting(
    bool is_default_engine) const {
  CHECK_IS_TEST();
  return is_default_engine ? default_filter_list_loaded_for_testing_
                           : additional_filter_list_loaded_for_testing_;
}

}  // namespace brave_shields
