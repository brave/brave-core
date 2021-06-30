/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_pref_store.h"

#include "base/logging.h"
#include "components/search_engines/default_search_manager.h"
#include "extensions/browser/extension_pref_value_map.h"

BraveExtensionPrefStore::BraveExtensionPrefStore(
    ExtensionPrefValueMap* extension_pref_value_map,
    bool incognito_pref_store)
    : ExtensionPrefStore(extension_pref_value_map, incognito_pref_store),
       extension_pref_value_map_(extension_pref_value_map),
      incognito_pref_store_(incognito_pref_store) {

  const base::Value* winner = extension_pref_value_map_->GetEffectivePrefValue(
      DefaultSearchManager::kDefaultSearchProviderDataPrefName, false, nullptr);
  if (winner) {
      LOG(ERROR) << __func__ << " #####(" << this << ") Copy Normal's url data!! ######";
      default_extension_search_provider_ = winner->Clone();
  }
}

BraveExtensionPrefStore::~BraveExtensionPrefStore() = default;

void BraveExtensionPrefStore::OnPrefValueChanged(const std::string& key) {
  ExtensionPrefStore::OnPrefValueChanged(key);

  if (key == DefaultSearchManager::kDefaultSearchProviderDataPrefName && incognito_pref_store_) {
    const base::Value* winner = extension_pref_value_map_->GetEffectivePrefValue(
        key, false, nullptr);
    if (winner) {
        LOG(ERROR) << __func__ << " #####(" << this << ") Copy Normal's url data!! ######";
        default_extension_search_provider_ = winner->Clone();
    } else {
        LOG(ERROR) << __func__ << " #####(" << this << ") Clear copied url data!! ######";
        default_extension_search_provider_ = base::Value();
    }
  }

  LOG(ERROR) << __func__ << " #####(" << this << ") incognito?: " << incognito_pref_store_;
  LOG(ERROR) << __func__ << " #####(" << this << ") key?: " << key;
}

bool BraveExtensionPrefStore::GetValue(const std::string& key,
                                  const base::Value** value) const {
  bool ret = ValueMapPrefStore::GetValue(key, value);

  if (incognito_pref_store_ && !ret && key == DefaultSearchManager::kDefaultSearchProviderDataPrefName) {
    if (!default_extension_search_provider_.is_none()) {
      if (value)
        *value = &default_extension_search_provider_;
      LOG(ERROR) << __func__ << " #####(" << this << ") return normal's data";
      return true;
    }
  }

  return ret;
}
