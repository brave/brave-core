/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using adblock::FilterList;

namespace brave_shields {

AdBlockRegionalServiceManager::AdBlockRegionalServiceManager(
    brave_component_updater::BraveComponent::Delegate* delegate)
    : delegate_(delegate),
      initialized_(false) {
  if (Init()) {
    initialized_ = true;
  }
}

AdBlockRegionalServiceManager::~AdBlockRegionalServiceManager() {
}

bool AdBlockRegionalServiceManager::Init() {
  DCHECK(!initialized_);
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&AdBlockRegionalServiceManager::StartRegionalServices,
                     base::Unretained(this)));
  return true;
}

void AdBlockRegionalServiceManager::StartRegionalServices() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state)
    return;

  // Enable the default regional list, but only do this once so that
  // user can override this setting in the future
  bool checked_default_region =
      local_state->GetBoolean(kAdBlockCheckedDefaultRegion);
  std::vector<FilterList>&  region_lists = FilterList::GetRegionalLists();
  if (!checked_default_region) {
    local_state->SetBoolean(kAdBlockCheckedDefaultRegion, true);
    auto it = brave_shields::FindAdBlockFilterListByLocale(
        region_lists, g_brave_browser_process->GetApplicationLocale());
    if (it == region_lists.end())
      return;
    EnableFilterList(it->uuid, true);
  }

  // Start all regional services associated with enabled filter lists
  base::AutoLock lock(regional_services_lock_);
  const base::DictionaryValue* regional_filters_dict =
      local_state->GetDictionary(kAdBlockRegionalFilters);
  for (base::DictionaryValue::Iterator it(*regional_filters_dict);
       !it.IsAtEnd(); it.Advance()) {
    const std::string uuid = it.key();
    bool enabled = false;
    const base::DictionaryValue* regional_filter_dict = nullptr;
    regional_filters_dict->GetDictionary(uuid, &regional_filter_dict);
    if (regional_filter_dict)
      regional_filter_dict->GetBoolean("enabled", &enabled);
    if (enabled) {
      auto regional_service = AdBlockRegionalServiceFactory(uuid, delegate_);
      regional_service->Start();
      regional_services_.insert(
          std::make_pair(uuid, std::move(regional_service)));
    }
  }
}

void AdBlockRegionalServiceManager::UpdateFilterListPrefs(
    const std::string& uuid,
    bool enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state)
    return;
  DictionaryPrefUpdate update(local_state, kAdBlockRegionalFilters);
  base::DictionaryValue* regional_filters_dict = update.Get();
  auto regional_filter_dict = std::make_unique<base::DictionaryValue>();
  regional_filter_dict->SetBoolean("enabled", enabled);
  regional_filters_dict->Set(uuid, std::move(regional_filter_dict));
}

bool AdBlockRegionalServiceManager::IsInitialized() const {
  return initialized_;
}

bool AdBlockRegionalServiceManager::Start() {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    regional_service.second->Start();
  }

  return true;
}

void AdBlockRegionalServiceManager::Stop() {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    regional_service.second->Stop();
  }
}

bool AdBlockRegionalServiceManager::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool* matching_exception_filter,
    bool* cancel_request_explicitly,
    std::string* mock_data_url) {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    if (!regional_service.second->ShouldStartRequest(
            url, resource_type, tab_host, matching_exception_filter,
            cancel_request_explicitly, mock_data_url)) {
      return false;
    }
    if (matching_exception_filter && *matching_exception_filter) {
      return true;
    }
  }

  return true;
}

void AdBlockRegionalServiceManager::EnableTag(const std::string& tag,
                                              bool enabled) {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    regional_service.second->EnableTag(tag, enabled);
  }
}

void AdBlockRegionalServiceManager::AddResources(
    const std::string& resources) {
  base::AutoLock lock(regional_services_lock_);
  for (const auto& regional_service : regional_services_) {
    regional_service.second->AddResources(resources);
  }
}

void AdBlockRegionalServiceManager::EnableFilterList(const std::string& uuid,
                                                     bool enabled) {
  DCHECK(!uuid.empty());

  // Enable or disable the specified filter list
  {
    base::AutoLock lock(regional_services_lock_);
    auto it = regional_services_.find(uuid);
    if (enabled) {
      DCHECK(it == regional_services_.end());
      auto regional_service = AdBlockRegionalServiceFactory(uuid, delegate_);
      regional_service->Start();
      regional_services_.insert(
          std::make_pair(uuid, std::move(regional_service)));
    } else {
      DCHECK(it != regional_services_.end());
      it->second->Stop();
      it->second->Unregister();
      regional_services_.erase(it);
    }
  }

  // Update preferences to reflect enabled/disabled state of specified
  // filter list
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&AdBlockRegionalServiceManager::UpdateFilterListPrefs,
                     base::Unretained(this), uuid, enabled));
}

base::Optional<base::Value>
AdBlockRegionalServiceManager::HostnameCosmeticResources(
        const std::string& hostname) {
  auto it = this->regional_services_.begin();
  if (it == this->regional_services_.end()) {
    return base::Optional<base::Value>();
  }
  base::Optional<base::Value> first_value =
      it->second->HostnameCosmeticResources(hostname);

  for ( ; it != this->regional_services_.end(); it++) {
    base::Optional<base::Value> next_value =
        it->second->HostnameCosmeticResources(hostname);
    if (first_value) {
      if (next_value) {
        MergeResourcesInto(&*first_value, &*next_value, false);
      }
    } else {
      first_value = std::move(next_value);
    }
  }

  return first_value;
}

base::Optional<base::Value>
AdBlockRegionalServiceManager::HiddenClassIdSelectors(
        const std::vector<std::string>& classes,
        const std::vector<std::string>& ids,
        const std::vector<std::string>& exceptions) {
  auto it = this->regional_services_.begin();
  if (it == this->regional_services_.end()) {
    return base::Optional<base::Value>();
  }
  base::Optional<base::Value> first_value =
      it->second->HiddenClassIdSelectors(classes, ids, exceptions);

  for ( ; it != this->regional_services_.end(); it++) {
    base::Optional<base::Value> next_value =
        it->second->HiddenClassIdSelectors(classes, ids, exceptions);
    if (first_value && first_value->is_list()) {
      if (next_value && next_value->is_list()) {
        for (auto i = next_value->GetList().begin();
                i < next_value->GetList().end();
                i++) {
          first_value->Append(std::move(*i));
        }
      }
    } else {
      first_value = std::move(next_value);
    }
  }

  return first_value;
}

// static
bool AdBlockRegionalServiceManager::IsSupportedLocale(
    const std::string& locale) {
  std::vector<FilterList>&  region_lists = FilterList::GetRegionalLists();
  return (brave_shields::FindAdBlockFilterListByLocale(region_lists, locale) !=
          region_lists.end());
}

// static
std::unique_ptr<base::ListValue>
AdBlockRegionalServiceManager::GetRegionalLists() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state)
    return nullptr;
  const base::DictionaryValue* regional_filters_dict =
      local_state->GetDictionary(kAdBlockRegionalFilters);

  auto list_value = std::make_unique<base::ListValue>();
  std::vector<FilterList>&  region_lists = FilterList::GetRegionalLists();
  for (const auto& region_list : region_lists) {
    // Most settings come directly from the region_lists vector, maintained in
    // the AdBlock module
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
    if (regional_filter_dict)
      regional_filter_dict->GetBoolean("enabled", &enabled);
    dict->SetBoolean("enabled", enabled);

    list_value->Append(std::move(dict));
  }

  return list_value;
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockRegionalServiceManager>
AdBlockRegionalServiceManagerFactory(BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockRegionalServiceManager>(delegate);
}

}  // namespace brave_shields
