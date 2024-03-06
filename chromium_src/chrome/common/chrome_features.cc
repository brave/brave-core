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

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
// Enables immersive fullscreen. The tab strip and toolbar are placed underneath
// the titlebar. The tab strip and toolbar can auto hide and reveal.
BASE_FEATURE(kImmersiveFullscreen,
             "ImmersiveFullscreen",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Moves the tab strip into the titlebar. kImmersiveFullscreen must be enabled
// for this feature to have an effect.
BASE_FEATURE(kImmersiveFullscreenTabs,
             "ImmersiveFullscreenTabs",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables immersive fullscreen mode for PWA windows. PWA windows will use
// immersive fullscreen mode if and only if both this and kImmersiveFullscreen
// are enabled. PWA windows currently do not use ImmersiveFullscreenTabs even if
// the feature is enabled.
BASE_FEATURE(kImmersiveFullscreenPWAs,
             "ImmersiveFullscreenPWAs",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kControlledFrame, base::FEATURE_DISABLED_BY_DEFAULT},
    {kKAnonymityService, base::FEATURE_DISABLED_BY_DEFAULT},
    {kKAnonymityServiceOHTTPRequests, base::FEATURE_DISABLED_BY_DEFAULT},
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kDesktopPWAsLinkCapturing, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
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
