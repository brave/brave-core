/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_engine.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using adblock::FilterList;
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
      task_runner_(task_runner),
      component_update_service_(cus) {}

void AdBlockRegionalServiceManager::Init(
    AdBlockResourceProvider* resource_provider,
    AdBlockRegionalCatalogProvider* catalog_provider) {
  DCHECK(!initialized_);
  resource_provider_ = resource_provider;
  catalog_provider_ = catalog_provider;
  catalog_provider_->LoadRegionalCatalog(
      base::BindOnce(&AdBlockRegionalServiceManager::OnRegionalCatalogLoaded,
                     weak_factory_.GetWeakPtr()));
  catalog_provider_->AddObserver(this);
  initialized_ = true;
}

AdBlockRegionalServiceManager::~AdBlockRegionalServiceManager() {
  catalog_provider_->RemoveObserver(this);
}

void AdBlockRegionalServiceManager::StartRegionalServices() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!local_state_)
    return;

  if (regional_catalog_.size() == 0) {
    return;
  }

  // Enable the default regional list, but only do this once so that
  // user can override this setting in the future
  bool checked_default_region =
      local_state_->GetBoolean(prefs::kAdBlockCheckedDefaultRegion);
  if (!checked_default_region) {
    local_state_->SetBoolean(prefs::kAdBlockCheckedDefaultRegion, true);
    auto it = brave_shields::FindAdBlockFilterListByLocale(regional_catalog_,
                                                           locale_);
    if (it == regional_catalog_.end())
      return;
    EnableFilterList(it->uuid, true);
  }

  const bool cookie_list_touched =
      local_state_->GetBoolean(prefs::kAdBlockCookieListSettingTouched);

  // Start all regional services associated with enabled filter lists
  base::AutoLock lock(regional_services_lock_);
  const base::DictionaryValue* regional_filters_dict =
      &base::Value::AsDictionaryValue(
          *local_state_->GetDictionary(prefs::kAdBlockRegionalFilters));

  base::Value regional_filters_dict_with_cookielist =
      base::Value(regional_filters_dict->Clone());
  if (base::FeatureList::IsEnabled(kBraveAdblockCookieListDefault) &&
      !cookie_list_touched) {
    auto cookie_list_entry = base::Value(base::Value::Type::DICTIONARY);
    cookie_list_entry.SetBoolKey("enabled", true);
    regional_filters_dict_with_cookielist.SetKey(kCookieListUuid,
                                                 std::move(cookie_list_entry));
  }

  for (const auto kv : regional_filters_dict_with_cookielist.DictItems()) {
    const std::string uuid = kv.first;
    bool enabled = false;
    const base::Value* regional_filter_dict =
        regional_filters_dict_with_cookielist.FindDictKey(uuid);
    if (regional_filter_dict) {
      enabled = regional_filter_dict->FindBoolKey("enabled").value_or(false);
    }
    if (enabled) {
      auto catalog_entry =
          brave_shields::FindAdBlockFilterListByUUID(regional_catalog_, uuid);
      auto existing_engine = regional_services_.find(uuid);
      // Iterating through locally enabled lists - don't disable any engines or
      // update existing engines with a potentially new catalog entry. They'll
      // be handled after a browser restart.
      if (catalog_entry != regional_catalog_.end() &&
          existing_engine == regional_services_.end()) {
        auto regional_filters_provider =
            std::make_unique<AdBlockRegionalFiltersProvider>(
                component_update_service_, *catalog_entry);
        auto regional_service =
            std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
                new AdBlockEngine(), base::OnTaskRunnerDeleter(task_runner_));
        auto observer =
            std::make_unique<AdBlockService::SourceProviderObserver>(
                regional_service->AsWeakPtr(), regional_filters_provider.get(),
                resource_provider_, task_runner_);
        regional_services_.insert({uuid, std::move(regional_service)});
        regional_filters_providers_.insert(
            {uuid, std::move(regional_filters_provider)});
        regional_source_observers_.insert({uuid, std::move(observer)});
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
  auto regional_filter_dict = base::Value(base::Value::Type::DICTIONARY);
  regional_filter_dict.SetBoolKey("enabled", enabled);
  regional_filters_dict->SetKey(uuid, std::move(regional_filter_dict));

  if (uuid == kCookieListUuid) {
    local_state_->SetBoolean(prefs::kAdBlockCookieListSettingTouched, true);
  }
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
    std::string* mock_data_url) {
  base::AutoLock lock(regional_services_lock_);

  for (const auto& regional_service : regional_services_) {
    regional_service.second->ShouldStartRequest(
        url, resource_type, tab_host, aggressive_blocking, did_match_rule,
        did_match_exception, did_match_important, mock_data_url);
    if (did_match_important && *did_match_important) {
      return;
    }
  }
}

absl::optional<std::string> AdBlockRegionalServiceManager::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  absl::optional<std::string> csp_directives = absl::nullopt;

  for (const auto& regional_service : regional_services_) {
    const auto directive =
        regional_service.second->GetCspDirectives(url, resource_type, tab_host);
    MergeCspDirectiveInto(directive, &csp_directives);
  }

  return csp_directives;
}

void AdBlockRegionalServiceManager::EnableTag(const std::string& tag,
                                              bool enabled) {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    regional_service.second->EnableTag(tag, enabled);
  }
}

void AdBlockRegionalServiceManager::AddResources(const std::string& resources) {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    regional_service.second->AddResources(resources);
  }
}

void AdBlockRegionalServiceManager::EnableFilterList(const std::string& uuid,
                                                     bool enabled) {
  DCHECK(!uuid.empty());
  auto catalog_entry =
      brave_shields::FindAdBlockFilterListByUUID(regional_catalog_, uuid);

  // Enable or disable the specified filter list
  base::AutoLock lock(regional_services_lock_);
  DCHECK(catalog_entry != regional_catalog_.end());
  auto it = regional_services_.find(uuid);
  if (enabled) {
    DCHECK(it == regional_services_.end());
    auto regional_filters_provider =
        std::make_unique<AdBlockRegionalFiltersProvider>(
            component_update_service_, *catalog_entry);
    auto regional_service =
        std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
            new AdBlockEngine(), base::OnTaskRunnerDeleter(task_runner_));
    auto observer = std::make_unique<AdBlockService::SourceProviderObserver>(
        regional_service->AsWeakPtr(), regional_filters_provider.get(),
        resource_provider_, task_runner_);
    regional_services_.insert({uuid, std::move(regional_service)});
    regional_filters_providers_.insert(
        {uuid, std::move(regional_filters_provider)});
    regional_source_observers_.insert({uuid, std::move(observer)});
  } else {
    auto observer = regional_source_observers_.find(uuid);
    DCHECK(observer != regional_source_observers_.end());
    regional_source_observers_.erase(observer);

    DCHECK(it != regional_services_.end());
    regional_services_.erase(it);

    auto it2 = regional_filters_providers_.find(uuid);
    DCHECK(it2 != regional_filters_providers_.end());
    std::move(*it2->second).Delete();
    regional_filters_providers_.erase(it2);
  }

  // Update preferences to reflect enabled/disabled state of specified
  // filter list
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&AdBlockRegionalServiceManager::UpdateFilterListPrefs,
                     weak_factory_.GetWeakPtr(), uuid, enabled));
}

absl::optional<base::Value> AdBlockRegionalServiceManager::UrlCosmeticResources(
    const std::string& url) {
  base::AutoLock lock(regional_services_lock_);
  auto it = regional_services_.begin();
  if (it == regional_services_.end()) {
    return absl::optional<base::Value>();
  }
  absl::optional<base::Value> first_value =
      it->second->UrlCosmeticResources(url);

  for (; it != regional_services_.end(); it++) {
    absl::optional<base::Value> next_value =
        it->second->UrlCosmeticResources(url);
    if (first_value) {
      if (next_value) {
        MergeResourcesInto(std::move(*next_value), &*first_value, false);
      }
    } else {
      first_value = std::move(next_value);
    }
  }

  return first_value;
}

base::Value AdBlockRegionalServiceManager::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  base::Value first_value(base::Value::Type::LIST);

  base::AutoLock lock(regional_services_lock_);
  for (auto it = regional_services_.begin(); it != regional_services_.end();
       it++) {
    base::Value next_value =
        it->second->HiddenClassIdSelectors(classes, ids, exceptions);
    DCHECK(next_value.is_list());

    for (auto i = next_value.GetListDeprecated().begin();
         i < next_value.GetListDeprecated().end(); i++) {
      first_value.Append(std::move(*i));
    }
  }

  return first_value;
}

void AdBlockRegionalServiceManager::SetRegionalCatalog(
    std::vector<adblock::FilterList> catalog) {
  regional_catalog_ = std::move(catalog);
  StartRegionalServices();
}

const std::vector<adblock::FilterList>&
AdBlockRegionalServiceManager::GetRegionalCatalog() {
  return regional_catalog_;
}

std::unique_ptr<base::ListValue>
AdBlockRegionalServiceManager::GetRegionalLists() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!local_state_)
    return nullptr;
  const base::DictionaryValue* regional_filters_dict =
      &base::Value::AsDictionaryValue(
          *local_state_->GetDictionary(prefs::kAdBlockRegionalFilters));

  const bool cookie_list_touched =
      local_state_->GetBoolean(prefs::kAdBlockCookieListSettingTouched);

  auto list_value = std::make_unique<base::ListValue>();
  for (const auto& region_list : regional_catalog_) {
    // Most settings come directly from the regional catalog from
    // https://github.com/brave/adblock-resources
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetString("uuid", region_list.uuid);
    dict->SetString("url", region_list.url);
    dict->SetString("title", region_list.title);
    dict->SetString("support_url", region_list.support_url);
    dict->SetString("component_id", region_list.component_id);
    dict->SetString("base64_public_key", region_list.base64_public_key);
    // However, the enabled/disabled flag is maintained in our
    // local_state preferences so retrieve it from there
    bool enabled = false;
    const base::DictionaryValue* regional_filter_dict = nullptr;
    regional_filters_dict->GetDictionary(region_list.uuid,
                                         &regional_filter_dict);
    if (region_list.uuid == kCookieListUuid &&
        base::FeatureList::IsEnabled(kBraveAdblockCookieListDefault) &&
        !cookie_list_touched) {
      enabled = true;
    } else if (regional_filter_dict) {
      enabled = regional_filter_dict->FindBoolKey("enabled").value_or(false);
    }
    dict->SetBoolean("enabled", enabled);

    list_value->Append(std::move(dict));
  }

  return list_value;
}

void AdBlockRegionalServiceManager::OnRegionalCatalogLoaded(
    const std::string& catalog_json) {
  SetRegionalCatalog(RegionalCatalogFromJSON(catalog_json));
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
