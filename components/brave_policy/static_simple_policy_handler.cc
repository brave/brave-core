// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_policy/static_simple_policy_handler.h"

#include "base/logging.h"
#include "components/prefs/pref_value_map.h"

namespace policy {

namespace {
// Global flag to bypass the cache for testing purposes.
bool g_cache_bypass = false;
}  // namespace

StaticSimplePolicyHandler::StaticSimplePolicyHandler(
    const char* policy_name,
    const char* pref_path,
    base::Value::Type value_type)
    : TypeCheckingPolicyHandler(policy_name, value_type),
      pref_path_(pref_path) {}

StaticSimplePolicyHandler::~StaticSimplePolicyHandler() = default;

// static
void StaticSimplePolicyHandler::SetCacheBypassForTesting(bool bypass) {
  g_cache_bypass = bypass;
}

void StaticSimplePolicyHandler::ApplyPolicySettings(const PolicyMap& policies,
                                                    PrefValueMap* prefs) {
  if (!pref_path_) {
    return;
  }

  // If cache bypass is enabled for testing, always read the current policy
  // but still cache values normally
  if (!g_cache_bypass && first_load_complete_) {
    // Use the cached value (which may be empty)
    if (cached_value_.has_value()) {
      prefs->SetValue(pref_path_, cached_value_->Clone());
    }
    return;
  }

  // Get the current policy value and cache it (or cache absence)
  const auto* value = policies.GetValueUnsafe(policy_name());
  if (value) {
    cached_value_ = value->Clone();
    prefs->SetValue(pref_path_, value->Clone());
  } else {
    cached_value_ = std::nullopt;
  }

  // Mark first load as complete regardless of whether a value was present
  first_load_complete_ = true;
}

}  // namespace policy
