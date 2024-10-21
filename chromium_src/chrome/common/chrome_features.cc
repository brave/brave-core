/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_features.h"

#include "src/chrome/common/chrome_features.cc"

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kChromeStructuredMetrics, base::FEATURE_DISABLED_BY_DEFAULT},
    {kHttpsFirstBalancedMode, base::FEATURE_DISABLED_BY_DEFAULT},
    {kKAnonymityService, base::FEATURE_DISABLED_BY_DEFAULT},
    {kKAnonymityServiceOHTTPRequests, base::FEATURE_DISABLED_BY_DEFAULT},
    {kSCTAuditing, base::FEATURE_DISABLED_BY_DEFAULT},
#if !BUILDFLAG(IS_ANDROID)
    {kTrustSafetySentimentSurvey, base::FEATURE_DISABLED_BY_DEFAULT},
    {kTrustSafetySentimentSurveyV2, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
#if BUILDFLAG(IS_MAC)
    {kImmersiveFullscreen, base::FEATURE_DISABLED_BY_DEFAULT},
    {kUseChromiumUpdater, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kShortcutsNotApps, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
}});

}  // namespace features
