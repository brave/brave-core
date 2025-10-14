/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/ad_block_only_mode/ad_block_only_mode_policies_utils.h"

#include "base/values.h"
#include "build/build_config.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"

void LoadAdBlockOnlyModePolicies(policy::PolicyBundle& bundle) {
  // This guard is necessary because the Ad Block Only mode policies below are
  // currently unsupported on iOS.
#if !BUILDFLAG(IS_IOS)
  policy::PolicyMap& policies = bundle.Get(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
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

  // Disable Brave Fingerprinting.
  policies.Set(policy::key::kDefaultBraveFingerprintingV2Setting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Set Brave Ads to block.
  policies.Set(policy::key::kDefaultBraveAdsSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_BLOCK), nullptr);

  // Disable Brave Cosmetic Filtering.
  policies.Set(policy::key::kDefaultBraveCosmeticFilteringSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Disable Brave Cookies.
  policies.Set(policy::key::kDefaultBraveCookiesSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Disable Brave HTTPS Upgrade.
  policies.Set(policy::key::kDefaultBraveHttpsUpgradeSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ASK), nullptr);

  // Disable Brave Referrers.
  policies.Set(policy::key::kDefaultBraveReferrersSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Disable Brave Remember 1P Storage.
  policies.Set(policy::key::kDefaultBraveRemember1PStorageSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);

  // Disable Brave Trackers.
  policies.Set(policy::key::kDefaultBraveTrackersSetting,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_BRAVE,
               base::Value(ContentSetting::CONTENT_SETTING_ALLOW), nullptr);
#endif  // !BUILDFLAG(IS_IOS)
}
