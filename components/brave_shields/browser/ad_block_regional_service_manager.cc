/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_engine.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using brave_shields::features::kBraveAdblockCookieListDefault;

namespace brave_shields {

AdBlockRegionalServiceManager::AdBlockRegionalServiceManager(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : local_state_(local_state),
      locale_(locale),
      initialized_(false),
      regional_engine_(
          std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
              new AdBlockEngine(),
              base::OnTaskRunnerDeleter(task_runner))),
      task_runner_(task_runner),
      component_update_service_(cus) {}

void AdBlockRegionalServiceManager::Init(
    AdBlockResourceProvider* resource_provider,
    AdBlockFilterListCatalogProvider* catalog_provider) {
  DCHECK(!initialized_);
  base::AutoLock lock(regional_services_lock_);
  resource_provider_ = resource_provider;
  catalog_provider_ = catalog_provider;
  catalog_provider_->LoadFilterListCatalog(
      base::BindOnce(&AdBlockRegionalServiceManager::OnFilterListCatalogLoaded,
                     weak_factory_.GetWeakPtr()));
  catalog_provider_->AddObserver(this);
  filters_manager_ = std::make_unique<AdBlockFiltersProviderManager>();
  regional_source_observer_ =
      std::make_unique<AdBlockService::SourceProviderObserver>(
          regional_engine_->AsWeakPtr(), filters_manager_.get(),
          resource_provider_, task_runner_);
  initialized_ = true;
}

AdBlockRegionalServiceManager::~AdBlockRegionalServiceManager() {
  catalog_provider_->RemoveObserver(this);
}

void AdBlockRegionalServiceManager::StartRegionalServices() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!local_state_)
    return;

  if (filter_list_catalog_.size() == 0) {
    return;
  }

  // Enable the default regional list, but only do this once so that
  // user can override this setting in the future
  bool checked_default_region =
      local_state_->GetBoolean(prefs::kAdBlockCheckedDefaultRegion);
  if (!checked_default_region) {
    local_state_->SetBoolean(prefs::kAdBlockCheckedDefaultRegion, true);
    auto it = brave_shields::FindAdBlockFilterListByLocale(filter_list_catalog_,
                                                           locale_);
    if (it == filter_list_catalog_.end())
      return;
    EnableFilterList(it->uuid, true);
  }

  const bool cookie_list_touched =
      local_state_->GetBoolean(prefs::kAdBlockCookieListSettingTouched);

  // Start all regional services associated with enabled filter lists
  base::AutoLock lock(regional_services_lock_);
  const auto& regional_filters_dict =
      local_state_->GetDict(prefs::kAdBlockRegionalFilters);

  auto regional_filters_dict_with_cookielist = regional_filters_dict.Clone();
  if (base::FeatureList::IsEnabled(kBraveAdblockCookieListDefault) &&
      !cookie_list_touched) {
    base::Value::Dict cookie_list_entry;
    cookie_list_entry.Set("enabled", true);
    regional_filters_dict_with_cookielist.Set(kCookieListUuid,
                                              std::move(cookie_list_entry));
  }

  for (const auto kv : regional_filters_dict_with_cookielist) {
    const std::string uuid = kv.first;
    bool enabled = false;
    const auto* regional_filter_dict =
        regional_filters_dict_with_cookielist.FindDict(uuid);
    if (regional_filter_dict) {
      enabled = regional_filter_dict->FindBool("enabled").value_or(false);
    }
    if (enabled) {
      auto catalog_entry = brave_shields::FindAdBlockFilterListByUUID(
          filter_list_catalog_, uuid);
      auto existing_provider = regional_filters_providers_.find(uuid);
      // Iterating through locally enabled lists - don't disable any providers
      // or update existing providers with a potentially new catalog entry.
      // They'll be handled after a browser restart.
      if (catalog_entry != filter_list_catalog_.end() &&
          existing_provider == regional_filters_providers_.end()) {
        auto regional_filters_provider =
            std::make_unique<AdBlockComponentFiltersProvider>(
                component_update_service_, *catalog_entry);
        filters_manager_->AddProvider(regional_filters_provider.get());
        regional_filters_providers_.insert(
            {uuid, std::move(regional_filters_provider)});
      }
    }
  }
}

void AdBlockRegionalServiceManager::UpdateFilterListPrefs(
    const std::string& uuid,
    bool enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!local_state_)
    return;
  DictionaryPrefUpdate update(local_state_, prefs::kAdBlockRegionalFilters);
  base::Value* regional_filters_dict = update.Get();
  base::Value::Dict regional_filter_dict;
  regional_filter_dict.Set("enabled", enabled);
  regional_filters_dict->GetDict().Set(uuid, std::move(regional_filter_dict));

  if (uuid == kCookieListUuid) {
    local_state_->SetBoolean(prefs::kAdBlockCookieListSettingTouched, true);
  }

  RecordP3ACookieListEnabled();
}

void AdBlockRegionalServiceManager::RecordP3ACookieListEnabled() {
  UMA_HISTOGRAM_BOOLEAN(kCookieListEnabledHistogram,
                        IsFilterListEnabled(kCookieListUuid));
}

bool AdBlockRegionalServiceManager::Start() {
  return true;
}

void AdBlockRegionalServiceManager::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* mock_data_url,
    std::string* rewritten_url) {
  base::AutoLock lock(regional_services_lock_);

  GURL request_url =
      rewritten_url && !rewritten_url->empty() ? GURL(*rewritten_url) : url;
  regional_engine_->ShouldStartRequest(
      request_url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, mock_data_url, rewritten_url);
}

absl::optional<std::string> AdBlockRegionalServiceManager::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  base::AutoLock lock(regional_services_lock_);
  return regional_engine_->GetCspDirectives(url, resource_type, tab_host);
}

void AdBlockRegionalServiceManager::UseResources(const std::string& resources) {
  base::AutoLock lock(regional_services_lock_);
  regional_engine_->UseResources(resources);
}

bool AdBlockRegionalServiceManager::IsFilterListAvailable(
    const std::string& uuid) const {
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);
  return catalog_entry != filter_list_catalog_.end();
}

bool AdBlockRegionalServiceManager::IsFilterListEnabled(
    const std::string& uuid) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!uuid.empty());
  DCHECK(local_state_);

  if (uuid == kCookieListUuid &&
      base::FeatureList::IsEnabled(kBraveAdblockCookieListDefault) &&
      !local_state_->GetBoolean(prefs::kAdBlockCookieListSettingTouched)) {
    return true;
  }

  const auto& regional_filters_dict =
      local_state_->GetDict(prefs::kAdBlockRegionalFilters);

  if (const auto* regional_filter_dict = regional_filters_dict.FindDict(uuid)) {
    return regional_filter_dict->FindBool("enabled").value_or(false);
  }

  return false;
}

void AdBlockRegionalServiceManager::EnableFilterList(const std::string& uuid,
                                                     bool enabled) {
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);

  // Enable or disable the specified filter list
  base::AutoLock lock(regional_services_lock_);
  DCHECK(catalog_entry != filter_list_catalog_.end());
  auto it = regional_filters_providers_.find(uuid);
  if (enabled) {
    DCHECK(it == regional_filters_providers_.end());
    auto regional_filters_provider =
        std::make_unique<AdBlockComponentFiltersProvider>(
            component_update_service_, *catalog_entry);
    filters_manager_->AddProvider(regional_filters_provider.get());
    regional_filters_providers_.insert(
        {uuid, std::move(regional_filters_provider)});
  } else {
    DCHECK(it != regional_filters_providers_.end());
    filters_manager_->RemoveProvider(it->second.get());
    std::move(*it->second).Delete();
    regional_filters_providers_.erase(it);
  }

  // Update preferences to reflect enabled/disabled state of specified
  // filter list
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockRegionalServiceManager::UpdateFilterListPrefs,
                     weak_factory_.GetWeakPtr(), uuid, enabled));
}

absl::optional<base::Value> AdBlockRegionalServiceManager::UrlCosmeticResources(
    const std::string& url) {
  base::AutoLock lock(regional_services_lock_);
  return regional_engine_->UrlCosmeticResources(url);
}

base::Value::List AdBlockRegionalServiceManager::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  base::AutoLock lock(regional_services_lock_);
  return regional_engine_->HiddenClassIdSelectors(classes, ids, exceptions);
}

void AdBlockRegionalServiceManager::SetFilterListCatalog(
    std::vector<FilterListCatalogEntry> catalog) {
  filter_list_catalog_ = std::move(catalog);
  StartRegionalServices();
  RecordP3ACookieListEnabled();
}

const std::vector<FilterListCatalogEntry>&
AdBlockRegionalServiceManager::GetFilterListCatalog() {
  return filter_list_catalog_;
}

base::Value::List AdBlockRegionalServiceManager::GetRegionalLists() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(local_state_);

  base::Value::List list;
  for (const auto& region_list : filter_list_catalog_) {
    // Most settings come directly from the regional catalog from
    // https://github.com/brave/adblock-resources
    base::Value::Dict dict;
    dict.Set("uuid", region_list.uuid);
    dict.Set("url", region_list.url);
    dict.Set("title", region_list.title);
    dict.Set("support_url", region_list.support_url);
    dict.Set("component_id", region_list.component_id);
    dict.Set("base64_public_key", region_list.base64_public_key);
    // However, the enabled/disabled flag is maintained in our
    // local_state preferences so retrieve it from there
    dict.Set("enabled", IsFilterListEnabled(region_list.uuid));
    list.Append(std::move(dict));
  }

  return list;
}

void AdBlockRegionalServiceManager::OnFilterListCatalogLoaded(
    const std::string& catalog_json) {
  SetFilterListCatalog(FilterListCatalogFromJSON(catalog_json));
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockRegionalServiceManager>
AdBlockRegionalServiceManagerFactory(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  return std::make_unique<AdBlockRegionalServiceManager>(local_state, locale,
                                                         cus, task_runner);
}

}  // namespace brave_shields
