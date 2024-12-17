/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_features.h"

#include "build/build_config.h"

namespace features {

// Cleanup Session Cookies on browser restart if Session Restore is enabled.
BASE_FEATURE(kBraveCleanupSessionCookiesOnSessionRestore,
             "BraveCleanupSessionCookiesOnSessionRestore",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Sanitize url before copying, replaces default ctrl+c hotkey for urls.
BASE_FEATURE(kBraveCopyCleanLinkByDefault,
             "brave-copy-clean-link-by-default",
#if BUILDFLAG(IS_MAC)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kBraveCopyCleanLinkFromJs,
             "BraveCopyCleanLinkFromJs",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Disable download warnings for dangerous files when Safe Browsing is
// disabled.
BASE_FEATURE(kBraveOverrideDownloadDangerLevel,
             "brave-override-download-danger-level",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Adds rounded corners and a drop shadow to the main web content area and side
// bar panels.
BASE_FEATURE(kBraveWebViewRoundedCorners,
             "brave-web-view-rounded-corners",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enable "day zero" experiment modifications, for potential user
// retention improvements.
BASE_FEATURE(kBraveDayZeroExperiment,
             "BraveDayZeroExperiment",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_ANDROID)
// Enable new onboarding on Android
BASE_FEATURE(kNewAndroidOnboarding,
             "NewAndroidOnboarding",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_ANDROID)

// The variant of the "day zero" experiment. i.e. A, B, C, D, etc.
const base::FeatureParam<std::string> kBraveDayZeroExperimentVariant{
    &kBraveDayZeroExperiment,
    /*name=*/"variant",
    /*default_value=*/""};

}  // namespace features
