/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/components/brave_origin/features.h"
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
                                       BraveOriginPrefMap pref_definitions,
                                       policy::PolicyService* policy_service)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      profile_id_(profile_id),
      policy_service_(policy_service),
      pref_definitions_(std::move(pref_definitions)) {
  CHECK(local_state_);
  CHECK(profile_prefs_);

  // Initialize local state policy preferences for this profile if needed
  InitializeLocalStatePolicyPrefs();
}

BraveOriginService::~BraveOriginService() = default;

void BraveOriginService::Shutdown() {}

bool BraveOriginService::IsBraveOriginUser() const {
  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
#if DCHECK_IS_ON()  // Debug builds only
  return base::FeatureList::IsEnabled(features::kBraveOrigin);
#else
  return false;  // Always disabled in release builds
#endif
}

bool BraveOriginService::IsPrefControlledByBraveOrigin(
    const std::string& pref_name) const {
  // BraveOrigin controlled = user is BraveOrigin AND pref is NOT externally
  // managed
  if (!IsBraveOriginUser()) {
    return false;
  }

  // Check if this is a valid BraveOrigin preference
  const BraveOriginPrefInfo* pref_info = GetPrefInfo(pref_name);
  if (!pref_info) {
    return false;
  }

  if (!local_state_) {
    return false;
  }

  // Check if the pref is externally managed
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(prefs::kBravePolicies);

  std::string externally_managed_key;
  if (pref_info->scope == BraveOriginPolicyScope::kGlobal) {
    externally_managed_key = pref_name + prefs::kExternallyManagedSuffix;
  } else {
    externally_managed_key =
        GetLocalStatePolicyPrefKey(profile_id_, pref_name) +
        prefs::kExternallyManagedSuffix;
  }

  const base::Value* managed_value = policies_dict.Find(externally_managed_key);
  bool is_externally_managed =
      managed_value && managed_value->is_bool() && managed_value->GetBool();

  // For profile-scoped prefs, also check the global externally managed flag
  // as a fallback (BraveOriginPolicyProvider stores them globally)
  if (!is_externally_managed &&
      pref_info->scope == BraveOriginPolicyScope::kProfile) {
    std::string global_externally_managed_key =
        pref_name + prefs::kExternallyManagedSuffix;
    const base::Value* global_managed_value =
        policies_dict.Find(global_externally_managed_key);
    is_externally_managed = global_managed_value &&
                            global_managed_value->is_bool() &&
                            global_managed_value->GetBool();
  }

  // BraveOrigin controls it if it's NOT externally managed
  return !is_externally_managed;
}

bool BraveOriginService::WasManagedBeforeBraveOrigin() const {
  if (!local_state_) {
    return false;
  }

  // Check if any policies were externally managed by looking for
  // .externally_managed flags in local state
  const base::Value::Dict& policies_dict =
      local_state_->GetDict(prefs::kBravePolicies);

  // Check both global and profile-scoped preferences
  for (const auto& [pref_name, pref_info] : pref_definitions_) {
    std::string externally_managed_key;
    if (pref_info.scope == BraveOriginPolicyScope::kGlobal) {
      externally_managed_key = pref_name + prefs::kExternallyManagedSuffix;
    } else {
      externally_managed_key =
          GetLocalStatePolicyPrefKey(profile_id_, pref_name) +
          prefs::kExternallyManagedSuffix;
    }

    const base::Value* managed_value =
        policies_dict.Find(externally_managed_key);
    if (managed_value && managed_value->is_bool() && managed_value->GetBool()) {
      return true;
    }
  }

  return false;
}

bool BraveOriginService::SetBraveOriginPolicyValue(const std::string& pref_name,
                                                   base::Value value) {
  const BraveOriginPrefInfo* pref_info = GetPrefInfo(pref_name);
  if (!pref_info) {
    return false;
  }

  // Set preferences based on their scope (user prefs)
  if (value.is_bool()) {
    // Only set user pref if it's user settable
    if (pref_info->scope == BraveOriginPolicyScope::kGlobal &&
        pref_info->user_settable && local_state_) {
      local_state_->SetBoolean(pref_name, value.GetBool());
    } else if (pref_info->scope == BraveOriginPolicyScope::kProfile &&
               pref_info->user_settable && profile_prefs_) {
      profile_prefs_->SetBoolean(pref_name, value.GetBool());
    }

    // Always update local state policy pref for policy provider access
    UpdateLocalStatePolicyPref(pref_name, value);
  }

  // Trigger policy refresh to update the policy system
  if (policy_service_) {
    policy_service_->RefreshPolicies(base::BindOnce([]() {}),
                                     policy::PolicyFetchReason::kUnspecified);
  }

  return true;
}

base::Value BraveOriginService::GetBraveOriginPrefValue(
    const std::string& pref_name) const {
  const BraveOriginPrefInfo* pref_info = GetPrefInfo(pref_name);
  if (!pref_info) {
    return base::Value();
  }

  // Check if we have a policy value stored in local state policy registry
  if (local_state_) {
    const base::Value::Dict& policies_dict =
        local_state_->GetDict(prefs::kBravePolicies);

    std::string policy_key;
    if (pref_info->scope == BraveOriginPolicyScope::kGlobal) {
      policy_key = pref_name;
    } else {
      policy_key = GetLocalStatePolicyPrefKey(profile_id_, pref_name);
    }

    const base::Value* policy_value = policies_dict.Find(policy_key);
    if (policy_value) {
      return policy_value->Clone();
    }
  }

  // Otherwise get from the appropriate pref service
  PrefService* pref_service =
      (pref_info->scope == BraveOriginPolicyScope::kGlobal) ? local_state_
                                                            : profile_prefs_;

  if (!pref_service) {
    return pref_info->default_value.Clone();
  }

  const PrefService::Preference* pref = pref_service->FindPreference(pref_name);
  if (pref) {
    return pref->GetValue()->Clone();
  } else {
    return pref_info->default_value.Clone();
  }
}

const BraveOriginPrefInfo* BraveOriginService::GetPrefInfo(
    const std::string& pref_name) const {
  return brave_origin::GetPrefInfo(pref_definitions_, pref_name);
}

// static
std::string BraveOriginService::GetLocalStatePolicyPrefKey(
    const std::string& profile_id,
    const std::string& pref_name) {
  // Build the key for within the dictionary: <profile-id>.<pref-name>
  return absl::StrFormat("%s.%s", profile_id, pref_name);
}

void BraveOriginService::InitializeLocalStatePolicyPrefs() {
  if (!local_state_ || !profile_prefs_) {
    return;
  }

  // Initialize local state policy prefs with default values for this profile
  for (const auto& [pref_name, pref_info] : pref_definitions_) {
    if (pref_info.scope == BraveOriginPolicyScope::kProfile) {
      std::string dict_key = GetLocalStatePolicyPrefKey(profile_id_, pref_name);

      // Check if value exists in the dictionary
      const base::Value::Dict& policies_dict =
          local_state_->GetDict(prefs::kBravePolicies);
      if (!policies_dict.Find(dict_key)) {
        // Set the value in the dictionary
        ScopedDictPrefUpdate update(local_state_, prefs::kBravePolicies);
        update->Set(dict_key, pref_info.default_value.Clone());
      }
    }
  }
}

void BraveOriginService::UpdateLocalStatePolicyPref(
    const std::string& pref_name,
    const base::Value& value) {
  const BraveOriginPrefInfo* pref_info = GetPrefInfo(pref_name);
  if (!pref_info || !local_state_) {
    return;
  }

  // Build the appropriate key based on preference scope
  std::string dict_key;
  if (pref_info->scope == BraveOriginPolicyScope::kProfile) {
    // Profile-scoped: <profile_id>.<pref_name>
    dict_key = GetLocalStatePolicyPrefKey(profile_id_, pref_name);
  } else {
    // Global-scoped: <pref_name>
    dict_key = pref_name;
  }

  // Update the value in the dictionary
  ScopedDictPrefUpdate update(local_state_, prefs::kBravePolicies);
  update->Set(dict_key, value.Clone());
}

// static
void BraveOriginService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // Register the dictionary for profile-scoped BraveOrigin policies
  // Keys will be: <profile-id>.<pref-name>
  registry->RegisterDictionaryPref(prefs::kBravePolicies);
}

}  // namespace brave_origin
