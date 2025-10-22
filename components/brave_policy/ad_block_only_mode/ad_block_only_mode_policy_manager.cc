/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/ad_block_only_mode/ad_block_only_mode_policy_manager.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/brave_policy/ad_block_only_mode/buildflags/buildflags.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"

namespace brave_policy {

// static
AdBlockOnlyModePolicyManager* AdBlockOnlyModePolicyManager::GetInstance() {
  static base::NoDestructor<AdBlockOnlyModePolicyManager> instance;
  return instance.get();
}

void AdBlockOnlyModePolicyManager::Init(PrefService* local_state) {
  CHECK(local_state) << "AdBlockOnlyModePolicyManager local state should exist";

  local_state_ = local_state;

  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      brave_shields::prefs::kAdBlockOnlyModeEnabled,
      base::BindRepeating(
          &AdBlockOnlyModePolicyManager::OnAdBlockOnlyModeChanged,
          base::Unretained(this)));

  OnAdBlockOnlyModeChanged();
}

void AdBlockOnlyModePolicyManager::Shutdown() {
  pref_change_registrar_.RemoveAll();
  observers_.Clear();
  local_state_ = nullptr;
}

void AdBlockOnlyModePolicyManager::AddObserver(BravePolicyObserver* observer) {
  observers_.AddObserver(observer);

  if (local_state_) {
    // Notify the observer to fetch Ad Block Only mode policies.
    observer->OnBravePoliciesReady();
  }
}

void AdBlockOnlyModePolicyManager::RemoveObserver(
    BravePolicyObserver* observer) {
  observers_.RemoveObserver(observer);
}

AdBlockOnlyModePolicies AdBlockOnlyModePolicyManager::GetPolicies() const {
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kAdblockOnlyMode)) {
    return {};
  }

  if (!local_state_ || !local_state_->GetBoolean(
                           brave_shields::prefs::kAdBlockOnlyModeEnabled)) {
    return {};
  }

  return GetPoliciesImpl();
}

AdBlockOnlyModePolicyManager::AdBlockOnlyModePolicyManager() = default;

AdBlockOnlyModePolicyManager::~AdBlockOnlyModePolicyManager() = default;

void AdBlockOnlyModePolicyManager::OnAdBlockOnlyModeChanged() {
  observers_.Notify(&BravePolicyObserver::OnBravePoliciesReady);
}

AdBlockOnlyModePolicies AdBlockOnlyModePolicyManager::GetPoliciesImpl() const {
#if BUILDFLAG(ENABLE_AD_BLOCK_ONLY_MODE_POLICIES)
  AdBlockOnlyModePolicies policies;

  // Allow JavaScript globally.
  policies.emplace(policy::key::kDefaultJavaScriptSetting,
                   base::Value(CONTENT_SETTING_ALLOW));

  // Allow all cookies.
  policies.emplace(policy::key::kDefaultCookiesSetting,
                   base::Value(CONTENT_SETTING_ALLOW));

  // Do not block third-party cookies.
  policies.emplace(policy::key::kBlockThirdPartyCookies, base::Value(false));

  // Disable language fingerprinting reduction.
  policies.emplace(policy::key::kBraveReduceLanguageEnabled,
                   base::Value(false));

  // Disable De-AMP.
  policies.emplace(policy::key::kBraveDeAmpEnabled, base::Value(false));

  // Disable URL debouncing.
  policies.emplace(policy::key::kBraveDebouncingEnabled, base::Value(false));

  return policies;
#else
  return {};
#endif  // BUILDFLAG(ENABLE_AD_BLOCK_ONLY_MODE_POLICIES)
}

}  // namespace brave_policy
