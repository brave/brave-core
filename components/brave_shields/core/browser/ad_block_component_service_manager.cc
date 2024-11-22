// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/memory/raw_ref.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

using brave_shields::features::kBraveAdblockCookieListDefault;
using brave_shields::features::kBraveAdblockExperimentalListDefault;
using brave_shields::features::kBraveAdblockMobileNotificationsListDefault;

namespace brave_shields {

namespace {

typedef struct ListDefaultOverrideConstants {
  RAW_PTR_EXCLUSION const base::Feature& feature;
  std::string_view list_uuid;
} ListDefaultOverrideConstants;

constexpr ListDefaultOverrideConstants kCookieListConstants{
    .feature = kBraveAdblockCookieListDefault,
    .list_uuid = kCookieListUuid};

constexpr ListDefaultOverrideConstants kMobileNotificationsListConstants{
    .feature = kBraveAdblockMobileNotificationsListDefault,
    .list_uuid = kMobileNotificationsListUuid};

constexpr ListDefaultOverrideConstants kExperimentalListConstants{
    .feature = kBraveAdblockExperimentalListDefault,
    .list_uuid = kExperimentalListUuid};

constexpr ListDefaultOverrideConstants kOverrideConstants[] = {
    kCookieListConstants, kMobileNotificationsListConstants,
    kExperimentalListConstants};

}  // namespace

AdBlockComponentServiceManager::AdBlockComponentServiceManager(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    AdBlockFilterListCatalogProvider* catalog_provider,
    AdBlockListP3A* list_p3a)
    : local_state_(local_state),
      locale_(locale),
      component_update_service_(cus),
      catalog_provider_(catalog_provider),
      list_p3a_(list_p3a) {
  catalog_provider_->LoadFilterListCatalog(
      base::BindOnce(&AdBlockComponentServiceManager::OnFilterListCatalogLoaded,
                     weak_factory_.GetWeakPtr()));
  catalog_provider_->AddObserver(this);
}

AdBlockComponentServiceManager::~AdBlockComponentServiceManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  catalog_provider_->RemoveObserver(this);
}

// If the older logic was used, only the first regional list for a given locale
// might have been enabled. If so, make sure the user hasn't explicitly
// modified any of these locale-specific list settings, to determine if all
// should be enabled.
bool AdBlockComponentServiceManager::NeedsLocaleListsMigration(
    std::vector<std::reference_wrapper<FilterListCatalogEntry const>>
        locale_lists) {
  // This can only apply to locales with more than one available list
  if (locale_lists.size() <= 1) {
    return false;
  }

  if (!IsFilterListEnabled(locale_lists[0].get().uuid)) {
    return false;
  }

  for (size_t i = 1; i < locale_lists.size(); i++) {
    if (IsFilterListEnabled(locale_lists[i].get().uuid)) {
      return false;
    }
  }

  return true;
}

void AdBlockComponentServiceManager::StartRegionalServices() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_) {
    return;
  }

  if (filter_list_catalog_.size() == 0) {
    return;
  }

  // Enable the default regional lists, but only do this once so that user can
  // override this setting in the future
  bool checked_all_default_regions =
      local_state_->GetBoolean(prefs::kAdBlockCheckedAllDefaultRegions);

  if (!checked_all_default_regions) {
    bool checked_default_region =
        local_state_->GetBoolean(prefs::kAdBlockCheckedDefaultRegion);
    auto locale_lists = brave_shields::FindAdBlockFilterListsByLocale(
        filter_list_catalog_, locale_);

    if (!checked_default_region || NeedsLocaleListsMigration(locale_lists)) {
      for (auto const& entry : locale_lists) {
        EnableFilterList(entry.get().uuid, true);
      }
    }

    local_state_->SetBoolean(prefs::kAdBlockCheckedAllDefaultRegions, true);
  }

  // Start component services associated with enabled filter lists
  for (const auto& catalog_entry : filter_list_catalog_) {
    if (IsFilterListEnabled(catalog_entry.uuid)) {
      auto existing_provider =
          component_filters_providers_.find(catalog_entry.uuid);
      // Only check for new lists that are part of the catalog - don't touch any
      // existing providers to account for modified or removed catalog entries.
      // They'll be handled after a browser restart.
      if (existing_provider == component_filters_providers_.end()) {
        auto regional_filters_provider =
            std::make_unique<AdBlockComponentFiltersProvider>(
                component_update_service_, catalog_entry,
                catalog_entry.first_party_protections);
        component_filters_providers_.insert(
            {catalog_entry.uuid, std::move(regional_filters_provider)});
      }
    }
  }
}

void AdBlockComponentServiceManager::UpdateFilterListPrefs(
    const std::string& uuid,
    bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_) {
    return;
  }
  {
    ScopedDictPrefUpdate update(local_state_, prefs::kAdBlockRegionalFilters);
    base::Value::Dict regional_filter_dict;
    regional_filter_dict.Set("enabled", enabled);
    update->Set(uuid, std::move(regional_filter_dict));
  }

  RecordP3ACookieListEnabled();
  list_p3a_->ReportFilterListUsage();
}

void AdBlockComponentServiceManager::RecordP3ACookieListEnabled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_BOOLEAN(kCookieListEnabledHistogram,
                        IsFilterListEnabled(kCookieListUuid));
}

bool AdBlockComponentServiceManager::IsFilterListAvailable(
    const std::string& uuid) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);
  return catalog_entry != filter_list_catalog_.end();
}

base::FilePath AdBlockComponentServiceManager::GetFilterSetPath(
    const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!uuid.empty());
  auto it = component_filters_providers_.find(uuid);

  if (it == component_filters_providers_.end()) {
    return base::FilePath();
  }

  return it->second->GetFilterSetPath();
}

bool AdBlockComponentServiceManager::IsFilterListEnabled(
    const std::string& uuid) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!uuid.empty());
  DCHECK(local_state_);

  // Retrieve user's setting for the list from preferences
  const auto& regional_filters_dict =
      local_state_->GetDict(prefs::kAdBlockRegionalFilters);
  const auto* user_pref_dict = regional_filters_dict.FindDict(uuid);
  const bool list_touched = user_pref_dict;

  // Apply feature overrides from Griffin without overriding user preference
  for (const auto& constants : kOverrideConstants) {
    if (uuid == constants.list_uuid &&
        base::FeatureList::IsEnabled(constants.feature) && !list_touched) {
      return true;
    }
  }

  // Apply overrides for lists with a `default_enabled` designation in the
  // catalog
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);
  if (catalog_entry != filter_list_catalog_.end()) {
    if (!catalog_entry->SupportsCurrentPlatform()) {
      return false;
    }
    // prefer any user setting for a default-enabled list, unless it's hidden
    if (catalog_entry->default_enabled &&
        (!list_touched || catalog_entry->hidden)) {
      return true;
    }
  }

  // Use the user's preference if there was no valid override
  if (user_pref_dict) {
    return user_pref_dict->FindBool("enabled").value_or(false);
  }

  // Otherwise the list is disabled
  return false;
}

void AdBlockComponentServiceManager::EnableFilterList(const std::string& uuid,
                                                      bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(filter_list_catalog_, uuid);

  if (catalog_entry == filter_list_catalog_.end()) {
    return;
  }
  // Enable or disable the specified filter list
  auto it = component_filters_providers_.find(uuid);
  if (enabled) {
    if (it != component_filters_providers_.end()) {
      return;
    }
    auto regional_filters_provider =
        std::make_unique<AdBlockComponentFiltersProvider>(
            component_update_service_, *catalog_entry,
            catalog_entry->first_party_protections);
    component_filters_providers_.insert(
        {uuid, std::move(regional_filters_provider)});
  } else {
    if (it == component_filters_providers_.end()) {
      return;
    }
    it->second->UnregisterComponent();
    component_filters_providers_.erase(it);
  }

  // Update preferences to reflect enabled/disabled state of specified
  // filter list
  UpdateFilterListPrefs(uuid, enabled);
}

void AdBlockComponentServiceManager::UpdateFilterLists(
    base::OnceCallback<void(bool)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If there are currently no components to update, then run the callback with
  // a success value in a future turn.
  if (component_filters_providers_.empty()) {
    std::move(callback).Run(true);
    return;
  }

  std::vector<std::string> component_ids = {
      kAdBlockResourceComponentId,
      kAdBlockFilterListCatalogComponentId,
  };

  for (const auto& [key, provider] : component_filters_providers_) {
    component_ids.push_back(provider->component_id());
  }

  auto on_updated = [](decltype(callback) cb, update_client::Error error) {
    std::move(cb).Run(error == update_client::Error::NONE ||
                      error == update_client::Error::UPDATE_IN_PROGRESS);
  };

  brave_component_updater::BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(
      component_ids, component_updater::OnDemandUpdater::Priority::FOREGROUND,
      base::BindOnce(on_updated, std::move(callback)));
}

void AdBlockComponentServiceManager::SetFilterListCatalog(
    std::vector<FilterListCatalogEntry> catalog) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  filter_list_catalog_ = std::move(catalog);
  StartRegionalServices();
  RecordP3ACookieListEnabled();

  list_p3a_->OnFilterListCatalogLoaded(filter_list_catalog_);
}

const std::vector<FilterListCatalogEntry>&
AdBlockComponentServiceManager::GetFilterListCatalog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return filter_list_catalog_;
}

base::Value::List AdBlockComponentServiceManager::GetRegionalLists() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(local_state_);

  base::Value::List list;
  for (const auto& region_list : filter_list_catalog_) {
    if (region_list.hidden || !region_list.SupportsCurrentPlatform()) {
      continue;
    }
    // Most settings come directly from the regional catalog from
    // https://github.com/brave/adblock-resources
    base::Value::Dict dict;
    dict.Set("uuid", region_list.uuid);
    dict.Set("url", region_list.url);
    dict.Set("title", region_list.title);
    dict.Set("desc", region_list.desc);
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

void AdBlockComponentServiceManager::OnFilterListCatalogLoaded(
    const std::string& catalog_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  SetFilterListCatalog(FilterListCatalogFromJSON(catalog_json));

  update_check_timer_.Start(
      FROM_HERE,
      base::Minutes(features::kComponentUpdateCheckIntervalMins.Get()),
      base::BindRepeating(&AdBlockComponentServiceManager::UpdateFilterLists,
                          weak_factory_.GetWeakPtr(), base::DoNothing()));
}

}  // namespace brave_shields
