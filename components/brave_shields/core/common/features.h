// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_FEATURES_H_

#include <cstddef>
#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_shields {
namespace features {
BASE_DECLARE_FEATURE(kAdBlockDefaultResourceUpdateInterval);
BASE_DECLARE_FEATURE(kBraveAdblockCnameUncloaking);
BASE_DECLARE_FEATURE(kBraveAdblockCollapseBlockedElements);
BASE_DECLARE_FEATURE(kBraveAdblockCookieListDefault);
BASE_DECLARE_FEATURE(kBraveAdblockCosmeticFiltering);
BASE_DECLARE_FEATURE(kBraveAdblockProceduralFiltering);
BASE_DECLARE_FEATURE(kBraveAdblockCspRules);
BASE_DECLARE_FEATURE(kBraveAdblockDefault1pBlocking);
BASE_DECLARE_FEATURE(kBraveAdblockMobileNotificationsListDefault);
BASE_DECLARE_FEATURE(kBraveAdblockExperimentalListDefault);
BASE_DECLARE_FEATURE(kBraveAdblockScriptletDebugLogs);
BASE_DECLARE_FEATURE(kBraveAdblockShowHiddenComponents);
BASE_DECLARE_FEATURE(kBraveDarkModeBlock);
BASE_DECLARE_FEATURE(kBraveDomainBlock);
BASE_DECLARE_FEATURE(kBraveDomainBlock1PES);
BASE_DECLARE_FEATURE(kBraveExtensionNetworkBlocking);
BASE_DECLARE_FEATURE(kBraveFarbling);
BASE_DECLARE_FEATURE(kBraveLocalhostAccessPermission);
BASE_DECLARE_FEATURE(kBraveReduceLanguage);
BASE_DECLARE_FEATURE(kBraveShredFeature);
BASE_DECLARE_FEATURE(kBraveShredCacheData);
BASE_DECLARE_FEATURE(kBraveShieldsContentSettingsIOS);
BASE_DECLARE_FEATURE(kBraveIOSDebugAdblock);
BASE_DECLARE_FEATURE(kBraveIOSEnableFarblingPlugins);
BASE_DECLARE_FEATURE(kBraveShowStrictFingerprintingMode);
BASE_DECLARE_FEATURE(kCosmeticFilteringExtraPerfMetrics);
BASE_DECLARE_FEATURE(kCosmeticFilteringJsPerformance);
BASE_DECLARE_FEATURE(kCosmeticFilteringSyncLoad);
BASE_DECLARE_FEATURE(kBlockAllCookiesToggle);
BASE_DECLARE_FEATURE(kCosmeticFilteringCustomScriptlets);
BASE_DECLARE_FEATURE(kBraveShieldsElementPicker);
BASE_DECLARE_FEATURE(kWebKitAdvancedPrivacyProtections);
extern const base::FeatureParam<int> kComponentUpdateCheckIntervalMins;
extern const base::FeatureParam<std::string>
    kCosmeticFilteringSubFrameFirstSelectorsPollingDelayMs;
extern const base::FeatureParam<std::string>
    kCosmeticFilteringswitchToSelectorsPollingThreshold;
extern const base::FeatureParam<std::string>
    kCosmeticFilteringFetchNewClassIdRulesThrottlingMs;
BASE_DECLARE_FEATURE(kAdblockOverrideRegexDiscardPolicy);
extern const base::FeatureParam<int>
    kAdblockOverrideRegexDiscardPolicyCleanupIntervalSec;
extern const base::FeatureParam<int>
    kAdblockOverrideRegexDiscardPolicyDiscardUnusedSec;

BASE_DECLARE_FEATURE(kAdblockOnlyMode);
// The number of times the shields are disabled before showing the Ad Block Only
// mode prompt in Shields Panel.
inline constexpr base::FeatureParam<int>
    kAdblockOnlyModePromptAfterShieldsDisabledCount{
        &kAdblockOnlyMode, "prompt_after_shields_disabled_count", 5};
// The time interval after which the Ad Block Only mode repeated reloads prompt
// is shown.
inline constexpr base::FeatureParam<base::TimeDelta>
    kAdblockOnlyModePromptAfterPageReloadsInterval{
        &kAdblockOnlyMode, "prompt_after_page_reloads_interval",
        base::Seconds(10)};
// The minimum number of times the page is reloaded before showing the Ad Block
// Only mode repeated reloads prompt.
inline constexpr base::FeatureParam<size_t>
    kAdblockOnlyModePromptAfterPageReloadsMin{
        &kAdblockOnlyMode, "prompt_after_page_reloads_min", 0};
// The maximum number of times the page is reloaded before showing the Ad Block
// Only mode repeated reloads prompt.
inline constexpr base::FeatureParam<size_t>
    kAdblockOnlyModePromptAfterPageReloadsMax{
        &kAdblockOnlyMode, "prompt_after_page_reloads_max", 0};

}  // namespace features
}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_FEATURES_H_
