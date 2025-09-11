/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_profile_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_policy {

BraveProfilePolicyProvider::BraveProfilePolicyProvider() = default;
BraveProfilePolicyProvider::~BraveProfilePolicyProvider() {
  brave_origin::BraveOriginPolicyManager::GetInstance()->RemoveObserver(this);
}

void BraveProfilePolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Register as BraveOriginPolicyManager observer.
  // This ensures feature flags and local state are available before policy
  // loading.
  brave_origin::BraveOriginPolicyManager::GetInstance()->AddObserver(this);
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

void BraveProfilePolicyProvider::OnBraveOriginPoliciesReady() {
  policies_ready_ = true;

  // Once we have BraveOrigin policies and a profile ID trigger Refresh policies
  if (!profile_id_.empty()) {
    RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
  }
}

policy::PolicyBundle BraveProfilePolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  if (brave_origin::IsBraveOriginEnabled()) {
    LoadBraveOriginPolicies(bundle);
  }

  return bundle;
}

void BraveProfilePolicyProvider::LoadBraveOriginPolicies(
    policy::PolicyBundle& bundle) {
  // Only process if we have a profile ID
  if (profile_id_.empty()) {
    return;  // No profile context yet
  }

  // Create policy map for Chrome domain
  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Get policy definitions and filter for profile scope only
  const auto& policy_definitions =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetDefinitions();
  for (const auto& [pref_name, policy_info] : policy_definitions) {
    // Only process profile scope preferences in this provider
    if (policy_info.scope == brave_origin::BraveOriginPolicyScope::kProfile) {
      LoadBraveOriginPolicy(bundle_policy_map, policy_info);
    }
  }
}

void BraveProfilePolicyProvider::LoadBraveOriginPolicy(
    policy::PolicyMap& bundle_policy_map,
    const brave_origin::BraveOriginPolicyInfo& policy_info) {
  // Get the policy value from BraveOriginPolicyManager (which handles local
  // state access)
  const base::Value& policy_value =
      brave_origin::BraveOriginPolicyManager::GetInstance()->GetPolicyValue(
          policy_info.pref_name, profile_id_);

  // Set the policy - the ConfigurationPolicyPrefStore will handle
  // converting this to the appropriate profile preference
  bundle_policy_map.Set(policy_info.policy_key, policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                        policy_value.Clone(), nullptr);
}

void BraveProfilePolicyProvider::SetProfileID(const std::string& profile_id) {
  profile_id_ = profile_id;

  // If policies are ready already and we now have a profile_Id, refresh the
  // policies
  if (policies_ready_) {
    RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
  }
}

std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider() {
  return std::make_unique<BraveProfilePolicyProvider>();
}

void SetBraveProfilePolicyProviderProfileID(
    policy::ConfigurationPolicyProvider* provider,
    const std::string& profile_id) {
  static_cast<BraveProfilePolicyProvider*>(provider)->SetProfileID(profile_id);
}

}  // namespace brave_policy
