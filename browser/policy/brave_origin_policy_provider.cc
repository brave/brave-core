/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/brave_origin_policy_provider.h"

#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/tor/pref_names.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

BraveOriginPolicyProvider::BraveOriginPolicyProvider(
    PrefService* local_state,
    policy::PolicyService* policy_service)
    : policy::ConfigurationPolicyProvider(),
      first_policies_loaded_(false),
      local_state_(local_state),
      policy_service_(policy_service) {
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

BraveOriginPolicyProvider::~BraveOriginPolicyProvider() = default;

void BraveOriginPolicyProvider::RefreshPolicies(
    policy::PolicyFetchReason reason) {
  policy::PolicyBundle bundle = LoadPolicies();
  first_policies_loaded_ = true;
  UpdatePolicy(std::move(bundle));
}

bool BraveOriginPolicyProvider::IsFirstPolicyLoadComplete(
    policy::PolicyDomain domain) const {
  return first_policies_loaded_;
}

policy::PolicyBundle BraveOriginPolicyProvider::LoadPolicies() {
  policy::PolicyBundle bundle;

  if (!local_state_) {
    return bundle;
  }

  // Get the current Brave Origin user status
  BraveOriginState* brave_origin_state = BraveOriginState::GetInstance();
  bool is_brave_origin_user =
      brave_origin_state && brave_origin_state->IsBraveOriginUser();

  if (!is_brave_origin_user) {
    // Clear tracked preferences when user is no longer BraveOrigin
    if (brave_origin_state) {
      brave_origin_state->ClearBraveOriginControlledPrefs();
    }
    return bundle;
  }

  // Check if the browser was managed before we apply BraveOrigin policies
  if (brave_origin_state && !first_policies_loaded_) {
    bool was_managed_before = false;
    if (policy_service_) {
      const policy::PolicyMap& existing_policies = policy_service_->GetPolicies(
          policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

      // Check if there are any existing enterprise policies
      was_managed_before = !existing_policies.empty();
    }
    brave_origin_state->SetWasManagedBeforeBraveOrigin(was_managed_before);
  }

  // Create policy map for Chrome domain
  policy::PolicyMap& policy_map = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  // Get user's stored policy settings
  const base::Value::Dict& policy_settings =
      local_state_->GetDict(prefs::kBraveOriginPolicySettings);

  // Set all BraveOrigin policies with user preferences or defaults
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveWaybackMachineEnabled,
      kBraveWaybackMachineEnabled, base::Value(false));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kTorDisabled,
      tor::prefs::kTorDisabled, base::Value(true));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveStatsPingEnabled,
      kStatsReportingEnabled, base::Value(false));
  SetBraveOriginPolicyWithPreference(policy_map, policy_settings,
                                     policy::key::kBraveP3ADisabled,
                                     p3a::kP3AEnabled, base::Value(false));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveRewardsDisabled,
      brave_rewards::prefs::kDisabledByPolicy, base::Value(true));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveWalletDisabled,
      brave_wallet::prefs::kDisabledByPolicy, base::Value(true));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveAIChatEnabled,
      ai_chat::prefs::kEnabledByPolicy, base::Value(false));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveSpeedreaderDisabled,
      speedreader::kSpeedreaderDisabledByPolicy, base::Value(true));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveNewsDisabled,
      brave_news::prefs::kBraveNewsDisabledByPolicy, base::Value(true));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveVPNDisabled,
      brave_vpn::prefs::kManagedBraveVPNDisabled, base::Value(true));
  SetBraveOriginPolicyWithPreference(
      policy_map, policy_settings, policy::key::kBraveTalkDisabled,
      kBraveTalkDisabledByPolicy, base::Value(true));

  return bundle;
}

bool BraveOriginPolicyProvider::IsPolicySetByExternalProvider(
    const std::string& policy_key) const {
  // We need to check the global policy service to see if this policy
  // is already set by other providers (registry, plist files, etc.)

  if (!policy_service_) {
    return false;
  }

  // Check if the policy is set by any provider
  const policy::PolicyMap& policies = policy_service_->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  const policy::PolicyMap::Entry* entry = policies.Get(policy_key);
  if (!entry) {
    return false;
  }

  // Check if the source is something other than our BraveOrigin source
  // If it's set by registry, plist, etc., we should respect that
  return entry->source != policy::POLICY_SOURCE_ENTERPRISE_DEFAULT;
}

void BraveOriginPolicyProvider::SetBraveOriginPolicyWithPreference(
    policy::PolicyMap& policy_map,
    const base::Value::Dict& policy_settings,
    const std::string& policy_key,
    const std::string& pref_name,
    const base::Value& default_value) {
  // Only set policy if not already set by external providers
  if (!IsPolicySetByExternalProvider(policy_key)) {
    // Check if user has a stored preference for this policy
    const base::Value* user_value = policy_settings.Find(pref_name);
    const base::Value& value_to_use = user_value ? *user_value : default_value;

    policy_map.Set(policy_key, policy::POLICY_LEVEL_MANDATORY,
                   policy::POLICY_SCOPE_MACHINE,
                   policy::POLICY_SOURCE_ENTERPRISE_DEFAULT,
                   value_to_use.Clone(), nullptr);

    // Track the preference that this policy controls in BraveOriginState
    BraveOriginState* brave_origin_state = BraveOriginState::GetInstance();
    if (brave_origin_state) {
      brave_origin_state->AddBraveOriginControlledPref(pref_name);
    }
  }
}

// static
void BraveOriginPolicyProvider::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(
      brave_origin::prefs::kBraveOriginPolicySettings);
}

}  // namespace brave_origin
