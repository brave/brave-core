/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

namespace {
// Helper function to check if a policy is controlled by BraveOrigin in a given
// policy service
bool IsPolicyControlledByBraveOrigin(policy::PolicyService* policy_service,
                                     const BraveOriginPolicyInfo* pref_info) {
  if (!policy_service || !pref_info) {
    return false;
  }

  const policy::PolicyMap& policies = policy_service->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  const policy::PolicyMap::Entry* entry = policies.Get(pref_info->policy_key);
  return entry && entry->source == policy::POLICY_SOURCE_BRAVE;
}
}  // namespace

BraveOriginService::BraveOriginService(
    PrefService* local_state,
    PrefService* profile_prefs,
    std::string_view profile_id,
    policy::PolicyService* profile_policy_service,
    policy::PolicyService* browser_policy_service)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      profile_id_(profile_id),
      profile_policy_service_(profile_policy_service),
      browser_policy_service_(browser_policy_service) {
  CHECK(local_state_);
  CHECK(profile_prefs_);
  CHECK(!profile_id_.empty());
}

BraveOriginService::~BraveOriginService() = default;

bool BraveOriginService::IsPrefControlledByBraveOrigin(
    std::string_view pref_name) const {
  if (!IsBraveOriginEnabled()) {
    return false;
  }

  // Check if this is a valid BraveOrigin preference
  const BraveOriginPolicyInfo* pref_info =
      BraveOriginPolicyManager::GetInstance()->GetPrefInfo(pref_name);
  if (!pref_info) {
    return false;
  }

  // Check if the policy is controlled by BraveOrigin in either browser or
  // profile policy service
  return IsPolicyControlledByBraveOrigin(browser_policy_service_, pref_info) ||
         IsPolicyControlledByBraveOrigin(profile_policy_service_, pref_info);
}

bool BraveOriginService::SetPolicyValue(std::string_view pref_name,
                                        bool value) {
  if (!IsBraveOriginEnabled()) {
    return false;
  }

  // Get policy info to access pref_key and user_settable
  auto* manager = BraveOriginPolicyManager::GetInstance();
  const BraveOriginPolicyInfo* pref_info = manager->GetPrefInfo(pref_name);
  if (!pref_info) {
    return false;
  }

  // Set the policy value in BraveOriginPolicyManager
  PrefService* target_prefs = nullptr;
  if (manager->IsBrowserPolicy(pref_info->pref_name)) {
    manager->SetPolicyValue(pref_name, value);
    target_prefs = local_state_;
  } else if (manager->IsProfilePolicy(pref_info->pref_name)) {
    manager->SetPolicyValue(pref_name, value, profile_id_);
    target_prefs = profile_prefs_;
  }
  CHECK(target_prefs);

  // Also set the corresponding pref value
  if (!pref_info->user_settable && value == pref_info->default_value) {
    target_prefs->ClearPref(pref_info->pref_name);
  } else {
    target_prefs->SetBoolean(pref_info->pref_name, value);
  }

  return true;
}

std::optional<bool> BraveOriginService::GetPolicyValue(
    std::string_view pref_name) const {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  if (manager->IsBrowserPolicy(pref_name)) {
    return manager->GetPolicyValue(pref_name);
  } else if (manager->IsProfilePolicy(pref_name)) {
    return manager->GetPolicyValue(pref_name, profile_id_);
  }
  return std::nullopt;
}

}  // namespace brave_origin
