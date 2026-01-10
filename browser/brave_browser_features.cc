/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_features.h"

#include "build/build_config.h"

namespace features {

BASE_FEATURE(kBraveNewTabPageRefreshEnabled,
             base::FEATURE_ENABLED_BY_DEFAULT);

// Cleanup Session Cookies on browser restart if Session Restore is enabled.
BASE_FEATURE(kBraveCleanupSessionCookiesOnSessionRestore,
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
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables rounded corners on the main content areas by default (when the user
// has not explicitly set the pref).
BASE_FEATURE(kBraveRoundedCornersByDefault,
             "brave-web-view-rounded-corners",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Disable download warnings for dangerous files when Safe Browsing is
// disabled.
BASE_FEATURE(kBraveOverrideDownloadDangerLevel,
             "brave-override-download-danger-level",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enable "day zero" experiment modifications, for potential user
// retention improvements.
BASE_FEATURE(kBraveDayZeroExperiment,
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(BRAVE_V8_ENABLE_DRUMBRAKE)
// Run WebAssembly code in the DrumBrake interpreter instead of the optimizing
// compiler. Automatically enabled when V8 is in jitless mode.
BASE_FEATURE(kBraveWebAssemblyJitless,
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // BUILDFLAG(BRAVE_V8_ENABLE_DRUMBRAKE)

// Controls V8 jitless mode. When enabled, V8 runs in jitless
// mode, which reduces performance but improves security.
BASE_FEATURE(kBraveV8JitlessMode,
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_ANDROID)
// Enable new onboarding on Android
BASE_FEATURE(kNewAndroidOnboarding,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enable dynamic colors on Android, which allows the app to adapt its
// color scheme based on the user's wallpaper and system theme.
// This feature is only available on Android 12 and above.
BASE_FEATURE(kBraveAndroidDynamicColors,
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enable fresh NTP display after idle expiration on Android.
// This feature allows showing a refreshed NTP when the app has been idle
// for a specified duration.
BASE_FEATURE(kBraveFreshNtpAfterIdleExperiment,
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_ANDROID)

// The variant of the "day zero" experiment. i.e. A, B, C, D, etc.
const base::FeatureParam<std::string> kBraveDayZeroExperimentVariant{
    &kBraveDayZeroExperiment,
    /*name=*/"variant",
    /*default_value=*/""};

#if BUILDFLAG(IS_ANDROID)
// The variant of the fresh NTP experiment. i.e. A, B, C, etc.
const base::FeatureParam<std::string> kBraveFreshNtpAfterIdleExperimentVariant{
    &kBraveFreshNtpAfterIdleExperiment,
    /*name=*/"variant",
    /*default_value=*/"A"};
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace features
