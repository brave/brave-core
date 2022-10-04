/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_features.h"

#define kDnsOverHttpsShowUiParam kDnsOverHttpsShowUiParamDisabled
#include "src/chrome/common/chrome_features.cc"
#undef kDnsOverHttpsShowUiParam

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kKAnonymityService, base::FEATURE_DISABLED_BY_DEFAULT},
    {kOmniboxTriggerForNoStatePrefetch, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPrivacyGuide2, base::FEATURE_DISABLED_BY_DEFAULT},
    {kSCTAuditing, base::FEATURE_DISABLED_BY_DEFAULT},
#if !BUILDFLAG(IS_ANDROID)
    {kTrustSafetySentimentSurvey, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    // Enable webui dark theme: @media (prefers-color-scheme: dark) is gated
    // on this feature.
    {kWebUIDarkMode, base::FEATURE_ENABLED_BY_DEFAULT},
}});

// Enable the DoH settings UI in chrome://settings/security on all platforms.
const base::FeatureParam<bool> kDnsOverHttpsShowUiParam{&kDnsOverHttps,
                                                        "ShowUi", true};

}  // namespace features
