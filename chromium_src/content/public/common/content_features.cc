/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/public/common/content_features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kAttributionFencedFrameReportingBeacon, base::FEATURE_DISABLED_BY_DEFAULT},
    {kCookieDeprecationFacilitatedTesting, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDigitalGoodsApi, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDIPS, base::FEATURE_DISABLED_BY_DEFAULT},
    {kFedCm, base::FEATURE_DISABLED_BY_DEFAULT},
    {kFirstPartySets, base::FEATURE_DISABLED_BY_DEFAULT},
    {kIdleDetection, base::FEATURE_DISABLED_BY_DEFAULT},
    {kLegacyTechReportEnableCookieIssueReports,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kLegacyTechReportTopLevelUrl, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNotificationTriggers, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPrivacySandboxAdsAPIsOverride, base::FEATURE_DISABLED_BY_DEFAULT},
    {kSignedHTTPExchange, base::FEATURE_DISABLED_BY_DEFAULT},
#if BUILDFLAG(IS_ANDROID)
    {kWebNfc, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kWebOTP, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
