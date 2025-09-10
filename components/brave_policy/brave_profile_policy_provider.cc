/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_profile_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_policy {

BraveProfilePolicyProvider::BraveProfilePolicyProvider(PrefService* local_state)
    : ad_block_only_mode_policy_provider_(local_state, *this) {}

BraveProfilePolicyProvider::~BraveProfilePolicyProvider() = default;

void BraveProfilePolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  ad_block_only_mode_policy_provider_.Init();

  // Trigger immediate policy loading to ensure policies are available in
  // chrome://policy
  // The actual policies will be conditionally added in LoadPolicies.
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

void BraveProfilePolicyProvider::RefreshPolicies(
    policy::PolicyFetchReason reason) {
  policy::PolicyBundle bundle = LoadPolicies();

  // Mark as loaded after successfully loading policies
  first_policies_loaded_ = true;

  UpdatePolicy(std::move(bundle));
}

bool BraveProfilePolicyProvider::IsFirstPolicyLoadComplete(
    policy::PolicyDomain domain) const {
  return first_policies_loaded_;
}

policy::PolicyBundle BraveProfilePolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  // Future work will add Brave Origin profile policies here

  ad_block_only_mode_policy_provider_.MaybeLoadPolicies(bundle);

  return bundle;
}

std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider(PrefService* local_state) {
  return std::make_unique<BraveProfilePolicyProvider>(local_state);
}

}  // namespace brave_policy
