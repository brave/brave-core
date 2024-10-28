/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace features {

BASE_DECLARE_FEATURE(kUseUpdatedNTP);
BASE_DECLARE_FEATURE(kBraveCleanupSessionCookiesOnSessionRestore);
BASE_DECLARE_FEATURE(kBraveCopyCleanLinkByDefault);
BASE_DECLARE_FEATURE(kBraveCopyCleanLinkFromJs);
BASE_DECLARE_FEATURE(kBraveOverrideDownloadDangerLevel);
BASE_DECLARE_FEATURE(kBraveWebViewRoundedCorners);
BASE_DECLARE_FEATURE(kBraveDayZeroExperiment);
#if BUILDFLAG(IS_ANDROID)
BASE_DECLARE_FEATURE(kNewAndroidOnboarding);
#endif  // BUILDFLAG(IS_ANDROID)

extern const base::FeatureParam<std::string> kBraveDayZeroExperimentVariant;

}  // namespace features

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_FEATURES_H_
