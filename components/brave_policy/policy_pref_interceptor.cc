/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_pref_interceptor.h"

#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "brave/components/brave_policy/features.h"
#include "brave/components/brave_policy/policy_pref_interceptor_list.h"
#include "components/prefs/pref_value_map.h"

namespace brave_policy {

namespace {

bool g_disable_caching_for_testing = false;

}  // namespace

PolicyPrefInterceptor::PolicyPrefInterceptor() = default;

PolicyPrefInterceptor::~PolicyPrefInterceptor() = default;

// static
void PolicyPrefInterceptor::DisableCachingForTesting() {
  g_disable_caching_for_testing = true;
}

void PolicyPrefInterceptor::InterceptPrefValues(PrefValueMap* pref_value_map,
                                                bool policies_initialized) {
  if (!pref_value_map || g_disable_caching_for_testing) {
    return;
  }

  // Check the instance first to avoid accessing the feature list before
  // FeatureList initialization is finalized.
  const bool feature_enabled =
      base::FeatureList::GetInstance() &&
      base::FeatureList::IsEnabled(features::kCacheNonDynamicPolicyPrefs);

  for (std::string_view pref_name :
       PolicyPrefInterceptorList::GetInstance()->GetPrefs()) {
    if (!policies_initialized) {
      // Policies are still loading. Keep refreshing the cache from the map
      // without touching the map itself, so the cache tracks the latest values
      // until policies are fully initialized.
      bool value = false;
      if (pref_value_map->GetBoolean(pref_name, &value)) {
        pref_cache_[pref_name] = value;
      } else {
        pref_cache_.erase(pref_name);
      }
    } else if (feature_enabled) {
      // Policies are initialized. Restore cached values, removing any pref not
      // present in the cache.
      if (const auto* cached_value = base::FindOrNull(pref_cache_, pref_name)) {
        pref_value_map->SetBoolean(pref_name, *cached_value);
      } else {
        pref_value_map->RemoveValue(pref_name);
      }
    }
  }
}

}  // namespace brave_policy
