/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/policy/brave_profile_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_constants.h"
#include "brave/components/brave_origin/brave_origin_pref_definitions.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/constants/pref_names.h"
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
    : policy::ConfigurationPolicyProvider(),
      first_policies_loaded_(false),
      local_state_(local_state) {
  CHECK(local_state_);
}

// BraveProfilePolicyProvider::~BraveProfilePolicyProvider() = default;
BraveProfilePolicyProvider::~BraveProfilePolicyProvider() {}

void BraveProfilePolicyProvider::Initialize(const std::string& profile_id,
                                            policy::SchemaRegistry* registry) {
  profile_id_ = profile_id;
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Set up pref watcher for brave_origin_policies changes
  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      brave_origin::kBraveOriginPolicies,
      base::BindRepeating(
          &BraveProfilePolicyProvider::OnBraveOriginPoliciesChanged,
          weak_factory_.GetWeakPtr()));

  // Trigger immediate policy loading to ensure policies are available in
  // chrome://policy
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

void BraveProfilePolicyProvider::Shutdown() {
  pref_change_registrar_.RemoveAll();
  // Call base class Shutdown
  policy::ConfigurationPolicyProvider::Shutdown();
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

  if (brave_origin::IsBraveOriginEnabled()) {
    LoadBraveOriginPolicies(bundle);
  }

  return bundle;
}

void BraveProfilePolicyProvider::LoadBraveOriginPolicies(
    policy::PolicyBundle& bundle) {
  // Create policy map for Chrome domain
  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  const base::Value::Dict& policies_dict =
      local_state_->GetDict(brave_origin::kBraveOriginPolicies);

  // Get pref definitions and filter for profile scope only
  const auto& pref_definitions =
      brave_origin::BraveOriginPrefDefinitions::GetInstance()->GetAll();
  for (const auto& [pref_name, pref_info] : pref_definitions) {
    // Only process profile scope preferences in this provider
    if (pref_info.scope == brave_origin::BraveOriginPolicyScope::kProfile) {
      LoadBraveOriginPolicy(bundle_policy_map, policies_dict, pref_info);
    }
  }
}

void BraveProfilePolicyProvider::LoadBraveOriginPolicy(
    policy::PolicyMap& bundle_policy_map,
    const base::Value::Dict& policies_dict,
    const brave_origin::BraveOriginPrefInfo& pref_info) {
  std::string brave_origin_pref_key =
      GetBraveOriginPrefKey(pref_info, profile_id_);
  const base::Value* policy_value = policies_dict.Find(brave_origin_pref_key);
  const base::Value& value_to_use =
      policy_value ? *policy_value : pref_info.default_value;

  // Set the policy - the ConfigurationPolicyPrefStore will handle
  // converting this to the appropriate profile preference
  bundle_policy_map.Set(pref_info.policy_key, policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_USER,
                        policy::POLICY_SOURCE_BRAVE_ORIGIN,
                        value_to_use.Clone(), nullptr);
}

void BraveProfilePolicyProvider::OnBraveOriginPoliciesChanged() {
  RefreshPolicies(policy::PolicyFetchReason::kUnspecified);
}

}  // namespace brave_policy
