/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/v8/buildflags/buildflags.h"

namespace features {

BASE_DECLARE_FEATURE(kBraveCleanupSessionCookiesOnSessionRestore);
BASE_DECLARE_FEATURE(kBraveCopyCleanLinkByDefault);
BASE_DECLARE_FEATURE(kBraveCopyCleanLinkFromJs);
BASE_DECLARE_FEATURE(kBraveOverrideDownloadDangerLevel);
BASE_DECLARE_FEATURE(kBraveRoundedCornersByDefault);
BASE_DECLARE_FEATURE(kBraveDayZeroExperiment);
#if BUILDFLAG(BRAVE_V8_ENABLE_DRUMBRAKE)
BASE_DECLARE_FEATURE(kBraveWebAssemblyJitless);
#endif  // BUILDFLAG(BRAVE_V8_ENABLE_DRUMBRAKE)
BASE_DECLARE_FEATURE(kBraveV8JitlessMode);
#if BUILDFLAG(IS_ANDROID)
BASE_DECLARE_FEATURE(kBraveAndroidDynamicColors);
BASE_DECLARE_FEATURE(kNewAndroidOnboarding);
BASE_DECLARE_FEATURE(kBraveFreshNtpAfterIdleExperiment);
#endif  // BUILDFLAG(IS_ANDROID)

extern const base::FeatureParam<std::string> kBraveDayZeroExperimentVariant;

#if BUILDFLAG(IS_ANDROID)
extern const base::FeatureParam<std::string>
    kBraveFreshNtpAfterIdleExperimentVariant;
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace features

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_
