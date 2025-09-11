/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_browser_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/prefs/pref_service.h"

namespace brave_policy {

BraveBrowserPolicyProvider::BraveBrowserPolicyProvider() = default;

BraveBrowserPolicyProvider::~BraveBrowserPolicyProvider() {
  brave_origin::BraveOriginPolicyManager::GetInstance()->RemoveObserver(this);
}

void BraveBrowserPolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Register as BraveOriginPolicyManager observer.
  // This ensures feature flags and local state are available before policy
  // loading.
  brave_origin::BraveOriginPolicyManager::GetInstance()->AddObserver(this);
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

void BraveBrowserPolicyProvider::OnBraveOriginPoliciesReady() {
  // Now that BraveOrigin policies are ready, trigger policy loading for the
  // first time.
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

policy::PolicyBundle BraveBrowserPolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  if (brave_origin::IsBraveOriginEnabled()) {
    LoadBraveOriginPolicies(bundle);
  }

  return bundle;
}

void BraveBrowserPolicyProvider::LoadBraveOriginPolicies(
    policy::PolicyBundle& bundle) {
  // Create policy map for Chrome domain
  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Get policy definitions and filter for global scope only
  const auto& policy_definitions =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetDefinitions();
  for (const auto& [pref_name, policy_info] : policy_definitions) {
    // Only process global scope preferences in this provider
    if (policy_info.scope == brave_origin::BraveOriginPolicyScope::kGlobal) {
      LoadBraveOriginPolicy(bundle_policy_map, policy_info);
    }
  }
}

void BraveBrowserPolicyProvider::LoadBraveOriginPolicy(
    policy::PolicyMap& bundle_policy_map,
    const brave_origin::BraveOriginPolicyInfo& policy_info) {
  // Get the policy value from BraveOriginPolicyManager (which handles local
  // state access) BraveBrowserPolicyProvider only handles global scope, so no
  // profile_id needed
  const base::Value& policy_value =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
          policy_info.pref_name, "");

  // Set the policy - the ConfigurationPolicyPrefStore will handle
  // converting this to the appropriate local state preference
  bundle_policy_map.Set(policy_info.policy_key, policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                        policy_value.Clone(), nullptr);
}

std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveBrowserPolicyProvider() {
  return std::make_unique<BraveBrowserPolicyProvider>();
}

}  // namespace brave_policy
