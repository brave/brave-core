/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_policy_provider.h"

#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_types.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

BraveOriginPolicyProvider::~BraveOriginPolicyProvider() = default;

void BraveOriginPolicyProvider::RefreshPolicies(
    policy::PolicyFetchReason reason) {
  policy::PolicyBundle bundle = LoadPolicies();
  first_policies_loaded_ = true;
  UpdatePolicy(std::move(bundle));
}

bool BraveOriginPolicyProvider::IsFirstPolicyLoadComplete(
    policy::PolicyDomain domain) const {
  return first_policies_loaded_;
}

BraveOriginPolicyProvider::BraveOriginPolicyProvider(PrefService* local_state)
    : first_policies_loaded_(false), local_state_(local_state) {
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

policy::PolicyBundle BraveOriginPolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  if (!local_state_) {
    return bundle;
  }

  // Get the current Brave Origin user status
  BraveOriginState* brave_origin_state = BraveOriginState::GetInstance();
  bool is_brave_origin_user =
      brave_origin_state && brave_origin_state->IsBraveOriginUser();

  if (!is_brave_origin_user) {
    return bundle;
  }

  // Load policies from preferences
  const base::Value::Dict& policy_settings =
      local_state_->GetDict(prefs::kBraveOriginPolicySettings);

  if (!policy_settings.empty()) {
    bundle
        .Get(policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME,
                                     std::string()))
        .LoadFrom(policy_settings, policy::POLICY_LEVEL_MANDATORY,
                  policy::POLICY_SCOPE_MACHINE,
                  policy::POLICY_SOURCE_BRAVE_ORIGIN);
  }

  return bundle;
}

}  // namespace brave_origin
