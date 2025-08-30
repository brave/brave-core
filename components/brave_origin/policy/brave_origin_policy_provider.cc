/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/policy/brave_origin_policy_provider.h"

#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_pref_definitions.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
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

void BraveOriginPolicyProvider::Initialize(const std::string& profile_id,
                                           policy::SchemaRegistry* registry) {
  profile_id_ = profile_id;
  // Call base class Init first
  policy::ConfigurationPolicyProvider::Init(registry);

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
  if (!IsBraveOriginEnabled()) {
    return bundle;
  }

  // Create policy map for Chrome domain
  policy::PolicyMap& bundle_policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  if (!local_state_) {
    return bundle;
  }

  // Get pref definitions from singleton
  const auto& pref_definitions =
      BraveOriginPrefDefinitions::GetInstance()->GetAll();
  for (const auto& [pref_name, pref_info] : pref_definitions) {
    LOG(ERROR) << "-----Load Policies: " << pref_info.policy_key
               << ", pref_name: " << pref_name;
    LoadPolicyForPref(bundle_policy_map, pref_info);
  }

  return bundle;
}

void BraveOriginPolicyProvider::LoadPolicyForPref(
    policy::PolicyMap& bundle_policy_map,
    const BraveOriginPrefInfo& pref_info) {
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(brave_origin::prefs::kBravePolicies);

  std::string brave_origin_pref_key =
      GetBraveOriginPrefKey(pref_info, profile_id_);
  const base::Value* policy_value = policies_dict.Find(brave_origin_pref_key);
  const base::Value& value_to_use =
      policy_value ? *policy_value : pref_info.default_value;

  if (!policy_value) {
    LOG(ERROR) << "No policy value found for pref: " << pref_info.pref_name
               << ", setting defaut value: " << pref_info.default_value;
  }

  bundle_policy_map.Set(pref_info.policy_key, policy::POLICY_LEVEL_MANDATORY,
                        policy::POLICY_SCOPE_MACHINE,
                        policy::POLICY_SOURCE_BRAVE_ORIGIN,
                        value_to_use.Clone(), nullptr);

  if (pref_info.scope == BraveOriginPolicyScope::kGlobal) {
    LOG(ERROR) << "Setting pref name: " << pref_info.pref_name
               << ", to value: " << value_to_use;
    local_state_->Set(pref_info.pref_name, value_to_use.Clone());
  }
}

}  // namespace brave_origin
