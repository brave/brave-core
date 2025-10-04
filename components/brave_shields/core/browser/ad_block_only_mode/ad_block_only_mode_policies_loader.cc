/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/core/browser/ad_block_only_mode/ad_block_only_mode_policies_loader.h"

#include "base/feature_list.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

namespace {

void SetPolicies(policy::PolicyMap& policies) {
  // This guard is necessary because the Ad Block Only mode policies below are
  // currently unsupported on iOS.
#if !BUILDFLAG(IS_IOS)
  // Allow JavaScript globally.
  policies.Set(policy::key::kDefaultJavaScriptSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Allow all cookies.
  policies.Set(policy::key::kDefaultCookiesSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Do not block third-party cookies.
  policies.Set(policy::key::kBlockThirdPartyCookies,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE, base::Value(false), nullptr);

  // Disable language fingerprinting reduction.
  policies.Set(policy::key::kBraveReduceLanguageEnabled,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE, base::Value(false), nullptr);

  // Disable De-AMP.
  policies.Set(policy::key::kBraveDeAmpEnabled, policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
               base::Value(false), nullptr);

  // Disable URL debouncing.
  policies.Set(policy::key::kBraveDebouncingEnabled,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE, base::Value(false), nullptr);
#endif  // !BUILDFLAG(IS_IOS)
}

}  // namespace

AdBlockOnlyModePoliciesLoader::AdBlockOnlyModePoliciesLoader(
    PrefService* local_state,
    policy::ConfigurationPolicyProvider& policy_provider)
    : local_state_(local_state), policy_provider_(policy_provider) {
  CHECK(local_state_);
}

AdBlockOnlyModePoliciesLoader::~AdBlockOnlyModePoliciesLoader() = default;

void AdBlockOnlyModePoliciesLoader::Init() {
  if (!base::FeatureList::IsEnabled(features::kAdblockOnlyMode)) {
    return;
  }

  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      prefs::kAdBlockOnlyModeEnabled,
      base::BindRepeating(
          &AdBlockOnlyModePoliciesLoader::OnAdBlockOnlyModeChanged,
          weak_factory_.GetWeakPtr()));
}

void AdBlockOnlyModePoliciesLoader::MaybeLoadPolicies(
    policy::PolicyBundle& bundle) {
  if (!base::FeatureList::IsEnabled(features::kAdblockOnlyMode)) {
    return;
  }

  if (!local_state_->GetBoolean(prefs::kAdBlockOnlyModeEnabled)) {
    return;
  }

  policy::PolicyMap& policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  SetPolicies(policies);
}

void AdBlockOnlyModePoliciesLoader::OnAdBlockOnlyModeChanged() {
  policy_provider_->RefreshPolicies(policy::PolicyFetchReason::kUserRequest);
}

}  // namespace brave_shields
