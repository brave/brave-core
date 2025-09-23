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

BraveOriginService::BraveOriginService(PrefService* local_state,
                                       PrefService* profile_prefs,
                                       std::string_view profile_id,
                                       policy::PolicyService* policy_service)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      profile_id_(profile_id),
      policy_service_(policy_service) {
  CHECK(local_state_);
  CHECK(profile_prefs_);
  CHECK(policy_service_);
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

  // Check if the active policy source is POLICY_SOURCE_BRAVE
  const policy::PolicyMap& policies = policy_service_->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  const policy::PolicyMap::Entry* entry = policies.Get(pref_info->policy_key);
  return entry && entry->source == policy::POLICY_SOURCE_BRAVE;
}

bool BraveOriginService::SetBrowserPolicyValue(std::string_view pref_name,
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
  manager->SetBrowserPolicyValue(pref_name, value);

  // Also set the corresponding pref value in local_state
  // If not user-settable and value equals default, clear the pref instead
  // We don't want a user to have a value set in local state if the value is not
  // user settable as they would be stuck with that value if they stopped being
  // a Brave Origin user
  if (!pref_info->user_settable && value == pref_info->default_value) {
    local_state_->ClearPref(pref_info->pref_name);
  } else {
    local_state_->SetBoolean(pref_info->pref_name, value);
  }

  return true;
}

bool BraveOriginService::SetProfilePolicyValue(std::string_view pref_name,
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
  manager->SetProfilePolicyValue(pref_name, value, profile_id_);

  // Also set the corresponding pref value in profile_prefs
  // If not user-settable and value equals default, clear the pref instead
  // We don't want a user to have a value set in profile prefs if the value is
  // not user settable as they would be stuck with that value if they stopped
  // being a Brave Origin user
  if (!pref_info->user_settable && value == pref_info->default_value) {
    profile_prefs_->ClearPref(pref_info->pref_name);
  } else {
    profile_prefs_->SetBoolean(pref_info->pref_name, value);
  }

  return true;
}

std::optional<bool> BraveOriginService::GetBrowserPrefValue(
    std::string_view pref_name) const {
  return BraveOriginPolicyManager::GetInstance()->GetPolicyValue(pref_name);
}

std::optional<bool> BraveOriginService::GetProfilePrefValue(
    std::string_view pref_name) const {
  return BraveOriginPolicyManager::GetInstance()->GetPolicyValue(pref_name,
                                                                 profile_id_);
}

}  // namespace brave_origin
