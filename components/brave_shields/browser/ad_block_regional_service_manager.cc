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
#include "brave/components/brave_shields/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"

using brave_shields::features::kBraveAdblockCookieListDefault;

namespace brave_shields {

AdBlockRegionalServiceManager::AdBlockRegionalServiceManager(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    AdBlockFilterListCatalogProvider* catalog_provider)
    : local_state_(local_state),
      locale_(locale),
      component_update_service_(cus),
      catalog_provider_(catalog_provider) {
  catalog_provider_->LoadFilterListCatalog(
      base::BindOnce(&AdBlockRegionalServiceManager::OnFilterListCatalogLoaded,
                     weak_factory_.GetWeakPtr()));
  catalog_provider_->AddObserver(this);
}

AdBlockRegionalServiceManager::~AdBlockRegionalServiceManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  catalog_provider_->RemoveObserver(this);
}

void AdBlockRegionalServiceManager::StartRegionalServices() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
        AdBlockFiltersProviderManager::GetInstance()->AddProvider(
            regional_filters_provider.get());
        regional_filters_providers_.insert(
            {uuid, std::move(regional_filters_provider)});
      }
    }
  }
}

void AdBlockRegionalServiceManager::UpdateFilterListPrefs(
    const std::string& uuid,
    bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_BOOLEAN(kCookieListEnabledHistogram,
                        IsFilterListEnabled(kCookieListUuid));
}

bool AdBlockRegionalServiceManager::IsFilterListAvailable(
    const std::string& uuid) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);
  return catalog_entry != filter_list_catalog_.end();
}

bool AdBlockRegionalServiceManager::IsFilterListEnabled(
    const std::string& uuid) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);

  // Enable or disable the specified filter list
  DCHECK(catalog_entry != filter_list_catalog_.end());
  auto it = regional_filters_providers_.find(uuid);
  if (enabled) {
    DCHECK(it == regional_filters_providers_.end());
    auto regional_filters_provider =
        std::make_unique<AdBlockComponentFiltersProvider>(
            component_update_service_, *catalog_entry);
    AdBlockFiltersProviderManager::GetInstance()->AddProvider(
        regional_filters_provider.get());
    regional_filters_providers_.insert(
        {uuid, std::move(regional_filters_provider)});
  } else {
    DCHECK(it != regional_filters_providers_.end());
    AdBlockFiltersProviderManager::GetInstance()->RemoveProvider(
        it->second.get());
    regional_filters_providers_.erase(it);
  }

  // Update preferences to reflect enabled/disabled state of specified
  // filter list
  UpdateFilterListPrefs(uuid, enabled);
}

void AdBlockRegionalServiceManager::SetFilterListCatalog(
    std::vector<FilterListCatalogEntry> catalog) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  filter_list_catalog_ = std::move(catalog);
  StartRegionalServices();
  RecordP3ACookieListEnabled();
}

const std::vector<FilterListCatalogEntry>&
AdBlockRegionalServiceManager::GetFilterListCatalog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return filter_list_catalog_;
}

base::Value::List AdBlockRegionalServiceManager::GetRegionalLists() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  SetFilterListCatalog(FilterListCatalogFromJSON(catalog_json));
}

}  // namespace brave_shields
