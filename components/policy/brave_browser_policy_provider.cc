/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/policy/brave_browser_policy_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_constants.h"
#include "brave/components/brave_origin/brave_origin_pref_definitions.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/prefs/pref_service.h"

namespace brave_policy {

BraveBrowserPolicyProvider::BraveBrowserPolicyProvider()
    : policy::ConfigurationPolicyProvider(), first_policies_loaded_(false) {}

BraveBrowserPolicyProvider::~BraveBrowserPolicyProvider() {}

void BraveBrowserPolicyProvider::Initialize(PrefService* local_state,
                                            policy::SchemaRegistry* registry) {
  CHECK(local_state);
  local_state_ = local_state;

  // Set up pref watcher for brave_origin_policies changes
  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      brave_origin::kBraveOriginPolicies,
      base::BindRepeating(
          &BraveBrowserPolicyProvider::OnBraveOriginPoliciesChanged,
          weak_factory_.GetWeakPtr()));

  // Trigger immediate policy loading for browser startup
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

void BraveBrowserPolicyProvider::Shutdown() {
  pref_change_registrar_.RemoveAll();
  // Call base class Shutdown
  policy::ConfigurationPolicyProvider::Shutdown();
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

  if (brave_origin::IsBraveOriginEnabled()) {
    LoadBraveOriginPolicies(bundle);
  }

  return bundle;
}

void BraveBrowserPolicyProvider::LoadBraveOriginPolicies(
    policy::PolicyBundle& bundle) {
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(brave_origin::kBraveOriginPolicies);

  // Create policy map for Chrome domain
  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Get pref definitions and filter for global scope only
  const auto& pref_definitions =
      brave_origin::BraveOriginPrefDefinitions::GetInstance()->GetAll();
  for (const auto& [pref_name, pref_info] : pref_definitions) {
    // Only process global scope preferences in this provider
    if (pref_info.scope == brave_origin::BraveOriginPolicyScope::kGlobal) {
      LoadBraveOriginPolicy(bundle_policy_map, policies_dict, pref_info);
    }
  }
}

void BraveBrowserPolicyProvider::LoadBraveOriginPolicy(
    policy::PolicyMap& bundle_policy_map,
    const base::Value::Dict& policies_dict,
    const brave_origin::BraveOriginPrefInfo& pref_info) {
  const base::Value* policy_value = policies_dict.Find(pref_info.policy_key);
  const base::Value& value_to_use =
      policy_value ? *policy_value : pref_info.default_value;

  // Set the policy - the ConfigurationPolicyPrefStore will handle
  // converting this to the appropriate local state preference
  bundle_policy_map.Set(pref_info.policy_key, policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_USER,
                        policy::POLICY_SOURCE_BRAVE_ORIGIN,
                        value_to_use.Clone(), nullptr);
}

void BraveBrowserPolicyProvider::OnBraveOriginPoliciesChanged() {
  RefreshPolicies(policy::PolicyFetchReason::kUnspecified);
}

}  // namespace brave_policy
