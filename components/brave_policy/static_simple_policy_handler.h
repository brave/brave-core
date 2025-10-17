// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_STATIC_SIMPLE_POLICY_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_STATIC_SIMPLE_POLICY_HANDLER_H_

#include <optional>

#include "base/values.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

namespace policy {

// A policy handler that caches the first policy value it encounters and
// continues to use that value for the rest of the session, even if the policy
// changes. This is useful for policies that do not support dynamic refresh.
class StaticSimplePolicyHandler : public TypeCheckingPolicyHandler {
 public:
  StaticSimplePolicyHandler(const char* policy_name,
                            const char* pref_path,
                            base::Value::Type value_type);
  StaticSimplePolicyHandler(const StaticSimplePolicyHandler&) = delete;
  StaticSimplePolicyHandler& operator=(const StaticSimplePolicyHandler&) =
      delete;
  ~StaticSimplePolicyHandler() override;

  // ConfigurationPolicyHandler methods:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;

  // Test-only method to bypass the cache. When enabled, the handler will
  // continue to cache values but won't use them.
  static void SetCacheBypassForTesting(bool bypass);

 private:
  // The DictionaryValue path of the preference the policy maps to.
  const char* pref_path_;

  // Cached policy value from the first time ApplyPolicySettings is called.
  // This optional may be empty, which represents that no policy was set.
  std::optional<base::Value> cached_value_;

  // Tracks whether the first load has completed. Once true, we always use
  // cached_value_ regardless of what the current policy says.
  bool first_load_complete_ = false;
};

}  // namespace policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_STATIC_SIMPLE_POLICY_HANDLER_H_
