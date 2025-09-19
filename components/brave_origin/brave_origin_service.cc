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

  BraveOriginPolicyManager::GetInstance()->SetBrowserPolicyValue(pref_name,
                                                                 value);
  return true;
}

bool BraveOriginService::SetProfilePolicyValue(std::string_view pref_name,
                                               bool value) {
  if (!IsBraveOriginEnabled()) {
    return false;
  }

  BraveOriginPolicyManager::GetInstance()->SetProfilePolicyValue(
      pref_name, value, profile_id_);
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
