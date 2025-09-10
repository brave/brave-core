/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/ad_block_only_mode_policy_provider.h"

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_policy {

AdBlockOnlyModePolicyProvider::AdBlockOnlyModePolicyProvider(
    PrefService* local_state,
    policy::ConfigurationPolicyProvider& policy_provider)
    : local_state_(local_state), policy_provider_(policy_provider) {
  CHECK(local_state_);
}

AdBlockOnlyModePolicyProvider::~AdBlockOnlyModePolicyProvider() = default;

void AdBlockOnlyModePolicyProvider::Init() {
  if (!brave_shields::IsAdblockOnlyModeFeatureEnabled()) {
    return;
  }

  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      brave_shields::prefs::kAdBlockAdblockOnlyModeGloballyDefaulted,
      base::BindRepeating(
          &AdBlockOnlyModePolicyProvider::OnAdBlockOnlyModeChanged,
          weak_factory_.GetWeakPtr()));
}

void AdBlockOnlyModePolicyProvider::MaybeLoadPolicies(
    policy::PolicyBundle& bundle) {
  if (!brave_shields::IsAdblockOnlyModeFeatureEnabled()) {
    return;
  }

  if (!local_state_->GetBoolean(
          brave_shields::prefs::kAdBlockAdblockOnlyModeGloballyDefaulted)) {
    return;
  }

  policy::PolicyMap& policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));

  policies.Set("DefaultJavaScriptSetting", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
               base::Value(1), nullptr);

  policies.Set("DefaultCookiesSetting", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
               base::Value(1), nullptr);

  policies.Set("BlockThirdPartyCookies", policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_BRAVE,
               base::Value(false), nullptr);
}

void AdBlockOnlyModePolicyProvider::OnAdBlockOnlyModeChanged() {
  // TODO(aseren): Add a reason for the policy refresh.
  policy_provider_->RefreshPolicies(policy::PolicyFetchReason::kUserRequest);
}

}  // namespace brave_policy
