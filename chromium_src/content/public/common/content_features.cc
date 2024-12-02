/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/public/common/content_features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace features {

// This is intended as a kill switch for the Idle Detection feature. To enable
// this feature, the experimental web platform features flag should be set,
// or the site should obtain an Origin Trial token.
BASE_FEATURE(kIdleDetection,
             "IdleDetection",
             base::FEATURE_DISABLED_BY_DEFAULT);

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kCookieDeprecationFacilitatedTesting, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDigitalGoodsApi, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDIPS, base::FEATURE_DISABLED_BY_DEFAULT},
    {kFedCm, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNotificationTriggers, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPrivacySandboxAdsAPIsOverride, base::FEATURE_DISABLED_BY_DEFAULT},
    {kServiceWorkerAutoPreload, base::FEATURE_DISABLED_BY_DEFAULT},
    {kWebIdentityDigitalCredentials, base::FEATURE_DISABLED_BY_DEFAULT},
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kPwaNavigationCapturing, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
#if BUILDFLAG(IS_ANDROID)
    {kWebNfc, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kWebOTP, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
