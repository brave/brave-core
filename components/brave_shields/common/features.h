// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_shields {
namespace features {
BASE_DECLARE_FEATURE(kBraveAdblockDefault1pBlocking);
BASE_DECLARE_FEATURE(kBraveAdblockCnameUncloaking);
BASE_DECLARE_FEATURE(kBraveAdblockCollapseBlockedElements);
BASE_DECLARE_FEATURE(kBraveAdblockCookieListDefault);
BASE_DECLARE_FEATURE(kBraveAdblockCookieListOptIn);
BASE_DECLARE_FEATURE(kBraveAdblockCosmeticFiltering);
BASE_DECLARE_FEATURE(kBraveAdblockCspRules);
BASE_DECLARE_FEATURE(kBraveAdblockMobileNotificationsListDefault);
BASE_DECLARE_FEATURE(kBraveAdblockScriptletDebugLogs);
BASE_DECLARE_FEATURE(kBraveDomainBlock);
BASE_DECLARE_FEATURE(kBraveDomainBlock1PES);
BASE_DECLARE_FEATURE(kBraveExtensionNetworkBlocking);
BASE_DECLARE_FEATURE(kBraveReduceLanguage);
BASE_DECLARE_FEATURE(kBraveLocalhostAccessPermission);
BASE_DECLARE_FEATURE(kBraveDarkModeBlock);
BASE_DECLARE_FEATURE(kCosmeticFilteringSyncLoad);
BASE_DECLARE_FEATURE(kCosmeticFilteringExtraPerfMetrics);
BASE_DECLARE_FEATURE(kCosmeticFilteringJsPerformance);
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

}  // namespace features
}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_FEATURES_H_
