/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/policy/brave_origin_policy_provider.h"

#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_prefs.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/constants/pref_names.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_origin {

BraveOriginPolicyProvider::BraveOriginPolicyProvider(PrefService* local_state)
    : policy::ConfigurationPolicyProvider(),
      first_policies_loaded_(false),
      local_state_(local_state) {}

// BraveOriginPolicyProvider::~BraveOriginPolicyProvider() = default;
BraveOriginPolicyProvider::~BraveOriginPolicyProvider() {}

void BraveOriginPolicyProvider::SetPolicyService(
    policy::PolicyService* policy_service) {
  policy_service_ = policy_service;
}

void BraveOriginPolicyProvider::Init(policy::SchemaRegistry* registry) {
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

  // Check which policies are already set by external providers (admin policies)
  // so we don't override them
  CheckExternallyManagedPolicies();

  // Trigger immediate policy loading to ensure policies are available in
  // chrome://policy
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

void BraveOriginPolicyProvider::Shutdown() {
  // Call base class Shutdown
  policy::ConfigurationPolicyProvider::Shutdown();
}

void BraveOriginPolicyProvider::RefreshPolicies(
    policy::PolicyFetchReason reason) {
  policy::PolicyBundle bundle = LoadPolicies();

  // Mark as loaded after successfully loading policies (or empty bundle if
  // user is not a BraveOrigin user, which is also a valid state)
  first_policies_loaded_ = true;

  UpdatePolicy(std::move(bundle));
}

bool BraveOriginPolicyProvider::IsFirstPolicyLoadComplete(
    policy::PolicyDomain domain) const {
  return first_policies_loaded_;
}

policy::PolicyBundle BraveOriginPolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  // Check if BraveOrigin feature is enabled
  bool is_brave_origin_user = IsBraveOriginEnabled();

  if (!is_brave_origin_user) {
    return bundle;
  }

  // Create policy map for Chrome domain
  policy::PolicyMap& policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  if (!local_state_) {
    return bundle;
  }

  // Get policy mappings and pref definitions from singleton
  const auto& policy_mappings =
      BraveOriginPrefs::GetInstance()->GetPolicyMappings();
  const auto& pref_definitions =
      BraveOriginPrefs::GetInstance()->GetPrefDefinitions();

  for (const auto& [policy_key, pref_name] : policy_mappings) {
    const BraveOriginPrefInfo* pref_info =
        brave_origin::GetPrefInfo(pref_definitions, pref_name);
    if (!pref_info) {
      continue;
    }
    if (pref_info->scope == BraveOriginPolicyScope::kProfile) {
      SetBraveOriginPolicyForPref(policy_map, policy_key, pref_name,
                                  local_state_);
    } else if (pref_info->scope == BraveOriginPolicyScope::kGlobal) {
      SetBraveOriginGlobalPolicyForPref(policy_map, policy_key, pref_name,
                                        local_state_);
    } else {
      continue;
    }
  }

  return bundle;
}

bool BraveOriginPolicyProvider::IsPolicySetByExternalProvider(
    const std::string& policy_key) const {
  if (!local_state_) {
    return false;
  }

  // Get the pref name for this policy key
  const auto& policy_mappings =
      BraveOriginPrefs::GetInstance()->GetPolicyMappings();
  auto it = policy_mappings.find(policy_key);
  if (it == policy_mappings.end()) {
    return false;
  }

  const std::string& pref_name = it->second;

  // Build the externally managed key
  std::string externally_managed_key =
      pref_name + brave_origin::prefs::kExternallyManagedSuffix;

  // Check local state for the external management flag
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(prefs::kBravePolicies);
  const base::Value* managed_value = policies_dict.Find(externally_managed_key);

  return managed_value && managed_value->is_bool() && managed_value->GetBool();
}

void BraveOriginPolicyProvider::SetBraveOriginPolicyForPref(
    policy::PolicyMap& policy_map,
    const std::string& policy_key,
    const std::string& pref_name,
    PrefService* local_state) {
  // Only set policy if not already set by external providers
  bool is_external = IsPolicySetByExternalProvider(policy_key);

  if (!is_external) {
    // Get the brave_policies dictionary and iterate through all keys
    const base::Value::Dict& policies_dict =
        local_state->GetDict(brave_origin::prefs::kBravePolicies);

    bool found_policy = false;
    // Look for keys that end with our pref_name (format: profile_id.pref_name)
    for (const auto item : policies_dict) {
      const std::string& dict_key = item.first;
      const base::Value& policy_value = item.second;

      // Check if this key is for our pref_name
      // Key format is: profile_id.pref_name
      // Find first dot (after profile_id) and extract everything after it
      size_t first_dot = dict_key.find('.');
      if (first_dot != std::string::npos) {
        std::string key_pref_name = dict_key.substr(first_dot + 1);
        if (key_pref_name == pref_name) {
          policy_map.Set(policy_key, policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_MACHINE,
                         policy::POLICY_SOURCE_PLATFORM, policy_value.Clone(),
                         nullptr);
          found_policy = true;
          break;  // Use first found value
        }
      }
    }

    if (!found_policy) {
      LOG(ERROR) << "No policy value found for pref: " << pref_name;
    }
  }
}

void BraveOriginPolicyProvider::SetBraveOriginGlobalPolicyForPref(
    policy::PolicyMap& policy_map,
    const std::string& policy_key,
    const std::string& pref_name,
    PrefService* local_state) {
  // Only set policy if not already set by external providers
  bool is_external = IsPolicySetByExternalProvider(policy_key);

  if (!is_external) {
    // For global policies, use the BraveOrigin default values from policy
    // definitions
    const auto& pref_definitions =
        BraveOriginPrefs::GetInstance()->GetPrefDefinitions();
    const BraveOriginPrefInfo* pref_info =
        brave_origin::GetPrefInfo(pref_definitions, pref_name);
    if (pref_info) {
      // Use the default value from the policy definition
      base::Value policy_value = pref_info->default_value.Clone();

      policy_map.Set(policy_key, policy::POLICY_LEVEL_MANDATORY,
                     policy::POLICY_SCOPE_MACHINE,
                     policy::POLICY_SOURCE_PLATFORM, policy_value.Clone(),
                     nullptr);

      // ALSO manually apply the policy to local_state pref since policy
      // handlers won't process profile-level policies for global prefs
      if (local_state) {
        local_state->Set(pref_name, std::move(policy_value));
      }
    } else {
      LOG(ERROR) << "No pref_info found for global pref: " << pref_name;
    }
  }
}

void BraveOriginPolicyProvider::CheckExternallyManagedPolicies() {
  if (!policy_service_ || !local_state_) {
    LOG(ERROR) << "No policy_service or local_state available for checking "
                  "external policies";
    return;
  }

  // Get all policies that are currently set by external providers
  const policy::PolicyMap& policies = policy_service_->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Check each BraveOrigin policy to see if it's already set externally
  const auto& policy_mappings =
      BraveOriginPrefs::GetInstance()->GetPolicyMappings();
  const auto& pref_definitions =
      BraveOriginPrefs::GetInstance()->GetPrefDefinitions();

  for (const auto& [policy_key, pref_name] : policy_mappings) {
    const policy::PolicyMap::Entry* entry = policies.Get(policy_key);

    // Get pref info to determine scope
    const BraveOriginPrefInfo* pref_info =
        brave_origin::GetPrefInfo(pref_definitions, pref_name);
    if (!pref_info) {
      continue;
    }

    // Build the externally managed key based on pref scope
    std::string externally_managed_key;
    if (pref_info->scope == BraveOriginPolicyScope::kGlobal) {
      externally_managed_key =
          pref_name + brave_origin::prefs::kExternallyManagedSuffix;
    } else {
      // For profile-scoped prefs, we need to check all profiles
      // Since we don't have profile info here, we'll store with a global flag
      // The actual profile-specific keys will be handled when
      // BraveOriginService instances are created for each profile
      externally_managed_key =
          pref_name + brave_origin::prefs::kExternallyManagedSuffix;
    }

    // Store the external management status in local state
    ScopedDictPrefUpdate update(local_state_, prefs::kBravePolicies);
    update->Set(externally_managed_key, entry != nullptr);
  }
}

bool BraveOriginPolicyProvider::IsBraveOriginEnabled() const {
  return base::FeatureList::IsEnabled(brave_origin::features::kBraveOrigin);
}

}  // namespace brave_origin
