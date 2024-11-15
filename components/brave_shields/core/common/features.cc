// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/features.h"

#include "base/feature_list.h"

namespace brave_shields::features {

BASE_FEATURE(kAdBlockDefaultResourceUpdateInterval,
             "AdBlockDefaultResourceUpdateInterval",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will block first-party requests that appear in a filter
// list when Shields is in "standard" blocking mode. When disabled, Brave will
// allow first-party requests in "standard" blocking mode regardless of whether
// or not they appear in a filter list.
BASE_FEATURE(kBraveAdblockDefault1pBlocking,
             "BraveAdblockDefault1pBlocking",
             base::FEATURE_DISABLED_BY_DEFAULT);
// When enabled, Brave will issue DNS queries for requests that the adblock
// engine has not blocked, then check them again with the original hostname
// substituted for any canonical name found.
BASE_FEATURE(kBraveAdblockCnameUncloaking,
             "BraveAdblockCnameUncloaking",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will apply HTML element collapsing to all images and
// iframes that initiate a blocked network request.
BASE_FEATURE(kBraveAdblockCollapseBlockedElements,
             "BraveAdblockCollapseBlockedElements",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will enable "Easylist-Cookie List" by default unless
// overridden by a locally set preference.
BASE_FEATURE(kBraveAdblockCookieListDefault,
             "BraveAdblockCookieListDefault",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will display a bubble inviting the user to turn on the
// "Easylist-Cookie List" filter.
BASE_FEATURE(kBraveAdblockCookieListOptIn,
             "BraveAdblockCookieListOptIn",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kBraveAdblockCosmeticFiltering,
             "BraveAdblockCosmeticFiltering",
             base::FEATURE_ENABLED_BY_DEFAULT);
// Brave will apply cosmetic filters with procedural operators like
// `:has-text(...)` and `:upward(...)`.
BASE_FEATURE(kBraveAdblockProceduralFiltering,
             "BraveAdblockProceduralFiltering",
             base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kBraveAdblockScriptletDebugLogs,
             "BraveAdblockScriptletDebugLogs",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kBraveAdblockCspRules,
             "BraveAdblockCspRules",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will enable "Fanboy's Mobile Notifications List" by
// default unless overridden by a locally set preference.
BASE_FEATURE(kBraveAdblockMobileNotificationsListDefault,
             "BraveAdblockMobileNotificationsListDefault",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will enable "Brave Experimental Adblock Rules" list by
// default unless overridden by a locally set preference.
// NOTE: this should only be turned on by default in Nightly and Beta.
BASE_FEATURE(kBraveAdblockExperimentalListDefault,
             "BraveAdblockExperimentalListDefault",
             base::FEATURE_DISABLED_BY_DEFAULT);
// When enabled, Brave will block domains listed in the user's selected adblock
// filters and present a security interstitial with choice to proceed and
// optionally whitelist the domain.
// Domain block filters look like this:
// ||ads.example.com^
BASE_FEATURE(kBraveDomainBlock,
             "BraveDomainBlock",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, Brave will attempt to enable 1PES mode in a standard blocking
// mode when a user visists a domain that is present in currently active adblock
// filters. 1PES will be enabled only if neither cookies nor localStorage data
// is stored for the website.
BASE_FEATURE(kBraveDomainBlock1PES,
             "BraveDomainBlock1PES",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, network requests initiated by extensions will be checked and
// potentially blocked by Brave Shields.
BASE_FEATURE(kBraveExtensionNetworkBlocking,
             "BraveExtensionNetworkBlocking",
             base::FEATURE_DISABLED_BY_DEFAULT);
// Enables Brave farbling (randomization of fingerprinting-susceptible WebAPIs).
BASE_FEATURE(kBraveFarbling, "BraveFarbling", base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, language headers and APIs may be altered by Brave Shields.
BASE_FEATURE(kBraveReduceLanguage,
             "BraveReduceLanguage",
             base::FEATURE_ENABLED_BY_DEFAULT);
// When enabled, brave shred feature will be available
BASE_FEATURE(kBraveShredFeature,
             "BraveShredFeature",
#if BUILDFLAG(IS_IOS)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif
// When enabled, brave shred will clear all cache data when shredding.
BASE_FEATURE(kBraveShredCacheData,
             "BraveShredCacheData",
#if BUILDFLAG(IS_IOS)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif
// When enabled, show Strict (aggressive) fingerprinting mode in Brave Shields.
BASE_FEATURE(kBraveShowStrictFingerprintingMode,
             "BraveShowStrictFingerprintingMode",
             base::FEATURE_DISABLED_BY_DEFAULT);
// when enabled, brave will prompt for permission on sites which want to connect
// to localhost.
BASE_FEATURE(kBraveLocalhostAccessPermission,
             "BraveLocalhostAccessPermission",
             base::FEATURE_DISABLED_BY_DEFAULT);
// When enabled, Brave will always report Light in Fingerprinting: Strict mode
BASE_FEATURE(kBraveDarkModeBlock,
             "BraveDarkModeBlock",
             base::FEATURE_ENABLED_BY_DEFAULT);
// load the cosmetic filter rules using sync ipc
BASE_FEATURE(kCosmeticFilteringSyncLoad,
             "CosmeticFilterSyncLoad",
             base::FEATURE_ENABLED_BY_DEFAULT);
// If the feature flag is on, we show the Block all Cookies toggle
BASE_FEATURE(kBlockAllCookiesToggle,
             "BlockAllCookiesToggle",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables extra TRACE_EVENTs in content filter js. The feature is
// primary designed for local debugging.
BASE_FEATURE(kCosmeticFilteringExtraPerfMetrics,
             "CosmeticFilteringExtraPerfMetrics",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kCosmeticFilteringJsPerformance,
             "CosmeticFilteringJsPerformance",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kCosmeticFilteringCustomScriptlets,
             "CosmeticFilteringCustomScriptlets",
             base::FEATURE_DISABLED_BY_DEFAULT);

constexpr base::FeatureParam<int> kComponentUpdateCheckIntervalMins{
    &kAdBlockDefaultResourceUpdateInterval, "update_interval_mins", 100};

constexpr base::FeatureParam<std::string>
    kCosmeticFilteringSubFrameFirstSelectorsPollingDelayMs{
        &kCosmeticFilteringJsPerformance, "subframes_first_query_delay_ms",
        "1000"};

constexpr base::FeatureParam<std::string>
    kCosmeticFilteringswitchToSelectorsPollingThreshold{
        &kCosmeticFilteringJsPerformance, "switch_to_polling_threshold",
        "1000"};

constexpr base::FeatureParam<std::string>
    kCosmeticFilteringFetchNewClassIdRulesThrottlingMs{
        &kCosmeticFilteringJsPerformance, "fetch_throttling_ms", "100"};

BASE_FEATURE(kAdblockOverrideRegexDiscardPolicy,
             "AdblockOverrideRegexDiscardPolicy",
             base::FEATURE_DISABLED_BY_DEFAULT);

constexpr base::FeatureParam<int>
    kAdblockOverrideRegexDiscardPolicyCleanupIntervalSec{
        &kAdblockOverrideRegexDiscardPolicy, "cleanup_interval_sec", 0};

constexpr base::FeatureParam<int>
    kAdblockOverrideRegexDiscardPolicyDiscardUnusedSec{
        &kAdblockOverrideRegexDiscardPolicy, "discard_unused_sec", 180};

}  // namespace brave_shields::features
