// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/debug/leak_annotations.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/task/bind_post_task.h"
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
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    bool engine_is_default)
    : on_resources_loaded_(std::move(on_resources_loaded)),
      engine_is_default_(engine_is_default),
      resource_provider_(resource_provider),
      filters_provider_manager_(filters_provider_manager),
      task_runner_(task_runner) {
  filters_provider_manager_->AddObserver(this);
  filters_provider_manager_->ForceNotifyObservers(engine_is_default);
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
  auto on_loaded_cb = base::BindOnce(
      &AdBlockService::SourceProviderObserver::OnFilterSetCallbackLoaded,
      weak_factory_.GetWeakPtr());
  filters_provider_manager_->LoadFilterSetForEngine(is_default_engine,
                                                    std::move(on_loaded_cb));
}

void AdBlockService::SourceProviderObserver::OnFilterSetCallbackLoaded(
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

void AdBlockService::SourceProviderObserver::OnFilterSetCreated(
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set) {
  TRACE_EVENT("brave.adblock", "OnFilterSetCreated");
  filter_set_ = std::move(filter_set);
  // multiple AddObserver calls are ignored
  resource_provider_->AddObserver(this);
  resource_provider_->LoadResources(base::BindOnce(
      &SourceProviderObserver::OnResourcesLoaded, weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::OnResourcesLoaded(
    AdblockResourceStorageBox storage) {
  on_resources_loaded_.Run(std::move(filter_set_), std::move(storage));
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
      engine_wrapper_(
          new AdBlockEngineWrapper(
              std::make_unique<AdBlockEngine>(true /* is_default */),
              std::make_unique<AdBlockEngine>(false /* is_default */)),
          base::OnTaskRunnerDeleter(GetTaskRunner())) {
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

  auto make_on_resources_loaded_callback = base::BindPostTask(
      task_runner_,
      base::BindRepeating(
          [](AdBlockEngine* engine,
             std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
             AdblockResourceStorageBox storage) {
            if (filter_set) {
              engine->Load(std::move(*filter_set), *storage);
            } else {
              engine->UseResources(*storage);
            }
          }));

  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      base::BindRepeating(make_on_resources_loaded_callback,
                          &engine_wrapper_->default_engine()),
      resource_provider_.get(), filters_provider_manager_.get(), task_runner_,
      true);
  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          base::BindRepeating(make_on_resources_loaded_callback,
                              &engine_wrapper_->additional_filters_engine()),
          resource_provider_.get(), filters_provider_manager_.get(),
          task_runner_, false);
}

AdBlockService::~AdBlockService() {
  // The engines are deleted on the task runner with SKIP_ON_SHUTDOWN trait,
  // therefore they leak during shutdown.
  ANNOTATE_LEAKING_OBJECT_PTR(engine_wrapper_.get());
}

void AdBlockService::EnableTag(const std::string& tag, bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Tags only need to be modified for the default engine.
  GetTaskRunner()->PostTask(
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
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::GetDebugInfo,
                     base::Unretained(&engine_wrapper_->default_engine())),
      base::BindOnce(&AdBlockService::OnGetDebugInfoFromDefaultEngine,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AdBlockService::DiscardRegex(uint64_t regex_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngineWrapper::DiscardRegex,
                     base::Unretained(engine_wrapper_.get()), regex_id));
}

void AdBlockService::SetupDiscardPolicy(
    const adblock::RegexManagerDiscardPolicy& policy) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngineWrapper::SetupDiscardPolicy,
                     base::Unretained(engine_wrapper_.get()), policy));
}

base::SequencedTaskRunner* AdBlockService::GetTaskRunner() {
  return task_runner_.get();
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

void AdBlockService::OnGetDebugInfoFromDefaultEngine(
    GetDebugInfoCallback callback,
    base::DictValue default_engine_debug_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          &AdBlockEngine::GetDebugInfo,
          base::Unretained(&engine_wrapper_->additional_filters_engine())),
      base::BindOnce(std::move(callback),
                     std::move(default_engine_debug_info)));
}

void AdBlockService::TagExistsForTest(const std::string& tag,
                                      base::OnceCallback<void(bool)> cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::TagExists,
                     base::Unretained(&engine_wrapper_->default_engine()), tag),
      std::move(cb));
}

}  // namespace brave_shields
