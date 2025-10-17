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

BraveBrowserPolicyProvider::~BraveBrowserPolicyProvider() = default;

void BraveBrowserPolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Register as BraveOriginPolicyManager observer.
  // This ensures feature flags and local state are available before policy
  // loading.
  policy_manager_observation_.Observe(
      brave_origin::BraveOriginPolicyManager::GetInstance());
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

void BraveBrowserPolicyProvider::OnBravePoliciesReady() {
  // Now that Brave policies are ready, trigger policy loading for the
  // first time.
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

void BraveBrowserPolicyProvider::OnBrowserPolicyChanged(
    std::string_view policy_key) {
  RefreshPolicies(policy::PolicyFetchReason::kUserRequest);
}

policy::PolicyBundle BraveBrowserPolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
#if DCHECK_IS_ON()  // Debug builds only
  if (brave_origin::IsBraveOriginEnabled()) {
    LoadBraveOriginPolicies(bundle);
  }
#else
  // Always disabled in release builds
#endif

  return bundle;
}

std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveBrowserPolicyProvider() {
  return std::make_unique<BraveBrowserPolicyProvider>();
}

void BraveBrowserPolicyProvider::LoadBraveOriginPolicies(
    policy::PolicyBundle& bundle) {
  // Create policy map for Chrome domain
  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Get all browser policies
  const auto policy_values =
      brave_origin::BraveOriginPolicyManager::GetInstance()
          ->GetAllBrowserPolicies();
  for (const auto& [policy_key, enabled] : policy_values) {
    LoadBraveOriginPolicy(bundle_policy_map, policy_key, enabled);
  }
}

void BraveBrowserPolicyProvider::LoadBraveOriginPolicy(
    policy::PolicyMap& bundle_policy_map,
    std::string_view policy_key,
    bool enabled) {
  // Set the policy - the ConfigurationPolicyPrefStore will handle
  // converting this to the appropriate local state preference
  bundle_policy_map.Set(std::string(policy_key), policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
                        base::Value(enabled), nullptr);
}

}  // namespace brave_policy
