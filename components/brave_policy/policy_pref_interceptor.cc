/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_pref_interceptor.h"

#include "base/containers/map_util.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_value_map.h"

namespace brave_policy {

namespace {

// Preference names that should not support dynamic refresh.
constexpr const char* kNonDynamicPrefs[] = {
    ai_chat::prefs::kEnabledByPolicy,
};

}  // namespace

PolicyPrefInterceptor::PolicyPrefInterceptor() = default;

PolicyPrefInterceptor::~PolicyPrefInterceptor() = default;

void PolicyPrefInterceptor::InterceptPrefValues(PrefValueMap* pref_value_map) {
  if (!pref_value_map) {
    return;
  }

  for (const auto* pref_name : kNonDynamicPrefs) {
    if (!initial_policies_loaded_) {
      // First time - cache the initial values.
      bool value = false;
      if (pref_value_map->GetBoolean(pref_name, &value)) {
        pref_cache_[pref_name] = value;
      }
    } else {
      // Subsequent calls - use cached values or remove if not cached.
      if (const auto* cached_value = base::FindOrNull(pref_cache_, pref_name)) {
        pref_value_map->SetBoolean(pref_name, *cached_value);
      } else {
        pref_value_map->RemoveValue(pref_name);
      }
    }
  }

  if (!initial_policies_loaded_) {
    initial_policies_loaded_ = true;
  }
}

}  // namespace brave_policy
