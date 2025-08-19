/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/components/brave_origin/brave_origin_pref_definitions.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/features.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_origin {

BraveOriginService::BraveOriginService(PrefService* local_state,
                                       PrefService* profile_prefs,
                                       const std::string& profile_id,
                                       policy::PolicyService* policy_service)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      profile_id_(profile_id),
      policy_service_(policy_service) {
  CHECK(local_state_);
  CHECK(profile_prefs_);
  CHECK(policy_service_);

  if (!IsBraveOriginEnabled()) {
    ClearUnsettablePrefs();
  }
}

BraveOriginService::~BraveOriginService() = default;

void BraveOriginService::Shutdown() {}

bool BraveOriginService::IsPrefControlledByBraveOrigin(
    const std::string& pref_name) const {
  // BraveOrigin controlled = user is BraveOrigin AND the active policy source
  // is POLICY_SOURCE_BRAVE_ORIGIN
  if (!IsBraveOriginEnabled()) {
    return false;
  }

  // Check if this is a valid BraveOrigin preference
  const BraveOriginPrefInfo* pref_info =
      BraveOriginPrefDefinitions::GetInstance()->GetPrefInfo(pref_name);
  if (!pref_info) {
    return false;
  }

  // Check if the active policy source is POLICY_SOURCE_BRAVE_ORIGIN
  const policy::PolicyMap& policies = policy_service_->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  const policy::PolicyMap::Entry* entry = policies.Get(pref_info->policy_key);
  return entry && entry->source == policy::POLICY_SOURCE_BRAVE_ORIGIN;
}

void BraveOriginService::ClearUnsettablePrefs() {
  if (!IsBraveOriginEnabled()) {
    const auto& pref_definitions =
        BraveOriginPrefDefinitions::GetInstance()->GetAll();
    for (const auto& [pref_name, pref_info] : pref_definitions) {
      if (pref_info.scope == BraveOriginPolicyScope::kGlobal &&
          !pref_info.user_settable) {
        local_state_->ClearPref(pref_name);
      } else if (pref_info.scope == BraveOriginPolicyScope::kProfile &&
                 !pref_info.user_settable) {
        profile_prefs_->ClearPref(pref_name);
      }
    }
  }
}

bool BraveOriginService::SetBraveOriginPolicyValue(const std::string& pref_name,
                                                   base::Value value) {
  if (!IsBraveOriginEnabled()) {
    return false;
  }
  const BraveOriginPrefInfo* pref_info =
      BraveOriginPrefDefinitions::GetInstance()->GetPrefInfo(pref_name);
  if (!pref_info) {
    return false;
  }

  // Set preferences based on their scope (user prefs)
  // We also set user unsettable prefs. They will be cleared
  // if the user is ever not an origin user.
  if (value.is_bool()) {
    // Only set user pref if it's user settable
    if (pref_info->scope == BraveOriginPolicyScope::kGlobal && local_state_) {
      LOG(ERROR) << "==Setting global BraveOrigin pref: " << pref_name << " to "
                 << value.GetBool();
      local_state_->SetBoolean(pref_name, value.GetBool());
    } else if (pref_info->scope == BraveOriginPolicyScope::kProfile) {
      LOG(ERROR) << "==Setting profile BraveOrigin pref: " << pref_name
                 << " to " << value.GetBool();
      profile_prefs_->SetBoolean(pref_name, value.GetBool());
    }

    // Always update local state policy pref for policy provider access
    UpdateLocalStatePolicyPref(pref_name, value);

    // Trigger policy refresh to update the policy system
    policy_service_->RefreshPolicies(base::BindOnce([]() {}),
                                     policy::PolicyFetchReason::kUnspecified);
    return true;
  }

  return false;
}

base::Value BraveOriginService::GetBraveOriginPrefValue(
    const std::string& pref_name) const {
  const BraveOriginPrefInfo* pref_info =
      BraveOriginPrefDefinitions::GetInstance()->GetPrefInfo(pref_name);
  if (!pref_info) {
    return base::Value();
  }

  // Check if we have a policy value stored in local state policy registry
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(prefs::kBravePolicies);

  std::string brave_origin_pref_key =
      GetBraveOriginPrefKey(*pref_info, profile_id_);
  const base::Value* policy_value = policies_dict.Find(brave_origin_pref_key);
  if (policy_value) {
    return policy_value->Clone();
  }

  // Otherwise get from the appropriate pref service
  PrefService* pref_service =
      (pref_info->scope == BraveOriginPolicyScope::kGlobal) ? local_state_
                                                            : profile_prefs_;
  const PrefService::Preference* pref = pref_service->FindPreference(pref_name);
  if (pref) {
    return pref->GetValue()->Clone();
  }

  return pref_info->default_value.Clone();
}

void BraveOriginService::UpdateLocalStatePolicyPref(
    const std::string& pref_name,
    const base::Value& value) {
  const BraveOriginPrefInfo* pref_info =
      BraveOriginPrefDefinitions::GetInstance()->GetPrefInfo(pref_name);
  if (!pref_info) {
    return;
  }

  // Update the value in the dictionary
  ScopedDictPrefUpdate update(local_state_, prefs::kBravePolicies);
  std::string brave_origin_pref_key =
      GetBraveOriginPrefKey(*pref_info, profile_id_);
  update->Set(brave_origin_pref_key, value.Clone());
}

// static
void BraveOriginService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // Register the dictionary for profile-scoped BraveOrigin policies
  // Keys will be: <profile-id>.<pref-name>
  registry->RegisterDictionaryPref(prefs::kBravePolicies);
}

}  // namespace brave_origin
