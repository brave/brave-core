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
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kDesktopPWAsLinkCapturing, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kPrivacyGuide3, base::FEATURE_DISABLED_BY_DEFAULT},
#if BUILDFLAG(IS_ANDROID)
    {kPrivacyGuideAndroid, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPrivacyGuideAndroidPostMVP, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kPrivacyGuidePreload, base::FEATURE_DISABLED_BY_DEFAULT},
#if BUILDFLAG(IS_ANDROID)
    {kPrivacyGuidePreloadAndroid, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kSCTAuditing, base::FEATURE_DISABLED_BY_DEFAULT},
#if !BUILDFLAG(IS_ANDROID)
    {kTrustSafetySentimentSurvey, base::FEATURE_DISABLED_BY_DEFAULT},
    {kTrustSafetySentimentSurveyV2, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
#if BUILDFLAG(IS_MAC)
    {kUseChromiumUpdater, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
}});

// Enable the DoH settings UI in chrome://settings/security on all platforms.
const base::FeatureParam<bool> kDnsOverHttpsShowUiParam{&kDnsOverHttps,
                                                        "ShowUi", true};

}  // namespace features
