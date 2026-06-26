/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_H_

#include <string_view>

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
  // refreshed. While `policies_initialized` is false, keeps the cache in sync
  // with the latest values in `pref_value_map` without modifying the map, since
  // policies may still be loading. Once `policies_initialized` is true,
  // overrides the map with the cached values, removing any pref that was not
  // cached.
  void InterceptPrefValues(PrefValueMap* pref_value_map,
                           bool policies_initialized);

  static void DisableCachingForTesting();

 private:
  // Cache of pref values that should remain stable across policy updates.
  base::flat_map<std::string_view, bool> pref_cache_;
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_H_
