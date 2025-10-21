/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_profile_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_policy/ad_block_only_mode/ad_block_only_mode_policy_manager.h"
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
BraveProfilePolicyProvider::~BraveProfilePolicyProvider() = default;

void BraveProfilePolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Register as BraveOriginPolicyManager observer.
  // This ensures feature flags and local state are available before policy
  // loading.
  brave_origin_observation_.Observe(
      brave_origin::BraveOriginPolicyManager::GetInstance());

  // Register as AdBlockOnlyModePolicyManager observer.
  ad_block_only_mode_policy_observation_.Observe(
      AdBlockOnlyModePolicyManager::GetInstance());
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

void BraveProfilePolicyProvider::OnBravePoliciesReady() {
  policies_ready_ = true;

  // Once we have Brave policies and a profile ID trigger Refresh policies
  if (!profile_id_.empty()) {
    // `OnBravePoliciesReady()` can be called multiple times, so we need to
    // choose the appropriate reason for the policies refresh basing on the
    // first policies load complete state.
    RefreshPolicies(first_policies_loaded_
                        ? policy::PolicyFetchReason::kUserRequest
                        : policy::PolicyFetchReason::kBrowserStart);
  }
}

policy::PolicyBundle BraveProfilePolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
#if !defined(OFFICIAL_BUILD)
  if (brave_origin::IsBraveOriginEnabled()) {
    LoadBraveOriginPolicies(bundle);
  }
#else
  // Always disabled in official builds
#endif

  MaybeLoadAdBlockOnlyModePolicies(bundle);

  return bundle;
}

std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider() {
  return std::make_unique<BraveProfilePolicyProvider>();
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

  // Get all profile policies
  const auto policy_values =
      brave_origin::BraveOriginPolicyManager::GetInstance()
          ->GetAllProfilePolicies(profile_id_);
  for (const auto& [policy_key, enabled] : policy_values) {
    LoadBraveOriginPolicy(bundle_policy_map, policy_key, enabled);
  }
}

void BraveProfilePolicyProvider::LoadBraveOriginPolicy(
    policy::PolicyMap& bundle_policy_map,
    std::string_view policy_key,
    bool enabled) {
  // Set the policy - the ConfigurationPolicyPrefStore will handle
  // converting this to the appropriate profile preference
  bundle_policy_map.Set(std::string(policy_key), policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                        base::Value(enabled), nullptr);
}

void BraveProfilePolicyProvider::MaybeLoadAdBlockOnlyModePolicies(
    policy::PolicyBundle& bundle) {
  auto ad_block_only_mode_policies =
      AdBlockOnlyModePolicyManager::GetInstance()->GetPolicies();
  // Ad Block Only mode policies can be empty if the feature is disabled or
  // the Ad Block Only mode is not enabled.
  if (ad_block_only_mode_policies.empty()) {
    return;
  }

  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  for (auto& [policy_key, value] : ad_block_only_mode_policies) {
    bundle_policy_map.Set(
        std::string(policy_key), policy::POLICY_LEVEL_MANDATORY,
        policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
        std::move(value), nullptr);
  }
}

void BraveProfilePolicyProvider::OnProfilePolicyChanged(
    std::string_view policy_key,
    std::string_view profile_id) {
  // Only update if this change is for our profile
  if (!profile_id_.empty() && profile_id == profile_id_) {
    RefreshPolicies(policy::PolicyFetchReason::kUserRequest);
  }
}

void BraveProfilePolicyProvider::SetProfileID(const std::string& profile_id) {
  profile_id_ = profile_id;

  // If policies are ready already and we now have a profile_Id, refresh the
  // policies
  if (policies_ready_) {
    RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
  }
}

}  // namespace brave_policy
