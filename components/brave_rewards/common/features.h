/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "build/build_config.h"

namespace brave_rewards::features {

#if BUILDFLAG(IS_ANDROID)
BASE_DECLARE_FEATURE(kBraveRewards);
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
BASE_DECLARE_FEATURE(kGeminiFeature);
#endif

BASE_DECLARE_FEATURE(kVerboseLoggingFeature);

BASE_DECLARE_FEATURE(kAllowUnsupportedWalletProvidersFeature);

BASE_DECLARE_FEATURE(kAllowSelfCustodyProvidersFeature);

BASE_DECLARE_FEATURE(kNewRewardsUIFeature);

BASE_DECLARE_FEATURE(kAnimatedBackgroundFeature);

BASE_DECLARE_FEATURE(kPlatformCreatorDetectionFeature);

}  // namespace brave_rewards::features

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_FEATURES_H_
