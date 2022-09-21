// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/common/features.h"

#include "base/feature_list.h"

namespace brave_shields {
namespace features {

// When enabled, Brave will block first-party requests that appear in a filter
// list when Shields is in "standard" blocking mode. When disabled, Brave will
// allow first-party requests in "standard" blocking mode regardless of whether
// or not they appear in a filter list.
const base::Feature kBraveAdblockDefault1pBlocking{
    "BraveAdblockDefault1pBlocking", base::FEATURE_DISABLED_BY_DEFAULT};
// When enabled, Brave will issue DNS queries for requests that the adblock
// engine has not blocked, then check them again with the original hostname
// substituted for any canonical name found.
const base::Feature kBraveAdblockCnameUncloaking{
    "BraveAdblockCnameUncloaking", base::FEATURE_ENABLED_BY_DEFAULT};
// When enabled, Brave will apply HTML element collapsing to all images and
// iframes that initiate a blocked network request.
const base::Feature kBraveAdblockCollapseBlockedElements{
    "BraveAdblockCollapseBlockedElements", base::FEATURE_ENABLED_BY_DEFAULT};
// When enabled, Brave will treat "Easylist-Cookie List" as a default,
// always-on list, overriding any locally set preference.
const base::Feature kBraveAdblockCookieListDefault{
    "BraveAdblockCookieListDefault", base::FEATURE_DISABLED_BY_DEFAULT};
// When enabled, Brave will display a bubble inviting the user to turn on the
// "Easylist-Cookie List" filter.
const base::Feature kBraveAdblockCookieListOptIn{
    "BraveAdblockCookieListOptIn", base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kBraveAdblockCosmeticFiltering{
    "BraveAdblockCosmeticFiltering", base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kBraveAdblockCosmeticFilteringChildFrames{
    "BraveAdblockCosmeticFilteringChildFrames",
    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kBraveAdblockCspRules{"BraveAdblockCspRules",
                                          base::FEATURE_ENABLED_BY_DEFAULT};
// When enabled, Brave will block domains listed in the user's selected adblock
// filters and present a security interstitial with choice to proceed and
// optionally whitelist the domain.
// Domain block filters look like this:
// ||ads.example.com^
const base::Feature kBraveDomainBlock{"BraveDomainBlock",
                                      base::FEATURE_ENABLED_BY_DEFAULT};
// When enabled, Brave will attempt to enable 1PES mode in a standard blocking
// mode when a user visists a domain that is present in currently active adblock
// filters. 1PES will be enabled only if neither cookies nor localStorage data
// is stored for the website.
const base::Feature kBraveDomainBlock1PES{"BraveDomainBlock1PES",
                                          base::FEATURE_ENABLED_BY_DEFAULT};
// When enabled, network requests initiated by extensions will be checked and
// potentially blocked by Brave Shields.
const base::Feature kBraveExtensionNetworkBlocking{
    "BraveExtensionNetworkBlocking", base::FEATURE_DISABLED_BY_DEFAULT};
// When enabled, language headers and APIs may be altered by Brave Shields.
const base::Feature kBraveReduceLanguage{"BraveReduceLanguage",
                                         base::FEATURE_ENABLED_BY_DEFAULT};
// When enabled, Brave will always report Light in Fingerprinting: Strict mode
const base::Feature kBraveDarkModeBlock{"BraveDarkModeBlock",
                                        base::FEATURE_ENABLED_BY_DEFAULT};
// load the cosmetic filter rules using sync ipc
const base::Feature kCosmeticFilteringSyncLoad{
    "CosmeticFilterSyncLoad", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables an extra TRAVE_EVENT in content filter scripts. The feature is
// primary designed for local debugging.
const base::Feature kCosmeticFilteringExtraPerfMetrics{
    "CosmeticFilteringExtraPerfMetrics", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kCosmeticFilteringJsPerformance{
    "CosmeticFilteringJsPerformance", base::FEATURE_DISABLED_BY_DEFAULT};

constexpr base::FeatureParam<std::string>
    kCosmeticFilteringfirstSelectorsPollingDelayMs{
        &kCosmeticFilteringJsPerformance, "first_selectors_query_delay_ms",
        "undefined"};

constexpr base::FeatureParam<std::string>
    kCosmeticFilteringswitchToSelectorsPollingThreshold{
        &kCosmeticFilteringJsPerformance, "switch_to_polling_threshold",
        "undefined"};

constexpr base::FeatureParam<std::string>
    kCosmeticFilteringFetchNewClassIdRulesThrottlingMs{
        &kCosmeticFilteringJsPerformance, "fetch_throttling_ms", "undefined"};

}  // namespace features
}  // namespace brave_shields
