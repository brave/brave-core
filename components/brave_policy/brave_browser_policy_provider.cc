/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_browser_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/prefs/pref_service.h"

namespace brave_policy {

BraveBrowserPolicyProvider::BraveBrowserPolicyProvider() = default;

BraveBrowserPolicyProvider::~BraveBrowserPolicyProvider() {}

void BraveBrowserPolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Trigger immediate policy loading to ensure policies are available in
  // chrome://policy
  // The actual policies will be conditionally added in LoadPolicies.
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

void BraveBrowserPolicyProvider::RefreshPolicies(
    policy::PolicyFetchReason reason) {
  policy::PolicyBundle bundle = LoadPolicies();

  // Mark as loaded after successfully loading policies
  first_policies_loaded_ = true;

  UpdatePolicy(std::move(bundle));
}

bool BraveBrowserPolicyProvider::IsFirstPolicyLoadComplete(
    policy::PolicyDomain domain) const {
  return first_policies_loaded_;
}

policy::PolicyBundle BraveBrowserPolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  // Future work will add policies here

  return bundle;
}

std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveBrowserPolicyProvider() {
  return std::make_unique<BraveBrowserPolicyProvider>();
}

}  // namespace brave_policy
