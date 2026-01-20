/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_H_

#include <string>

#include "base/component_export.h"
#include "base/containers/flat_map.h"

class PrefValueMap;

namespace brave_policy {

// PolicyPrefInterceptor is responsible for ensuring that policy pref values
// that don't support dynamic refresh never get changed at runtime.
class COMPONENT_EXPORT(POLICY_PREF_INTERCEPTOR) PolicyPrefInterceptor {
 public:
  PolicyPrefInterceptor();
  ~PolicyPrefInterceptor();

  PolicyPrefInterceptor(const PolicyPrefInterceptor&) = delete;
  PolicyPrefInterceptor& operator=(const PolicyPrefInterceptor&) = delete;

  // Intercepts pref value changes for prefs that should not be dynamically
  // refreshed. On first call for each pref, caches the initial value. On
  // subsequent calls, overrides the pref value with the cached value.
  void InterceptPrefValues(PrefValueMap* pref_value_map);

  static void DisableCachingForTesting();

 private:
  // Cache of pref values that should remain stable across policy updates.
  base::flat_map<std::string, bool> pref_cache_;

  // Tracks whether initial policies have been loaded and cached.
  bool initial_policies_loaded_ = false;
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_H_
