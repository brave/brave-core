/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/features.h"

namespace brave_rewards::features {

#if BUILDFLAG(IS_ANDROID)
#if defined(ARCH_CPU_X86_FAMILY) && defined(OFFICIAL_BUILD)
BASE_FEATURE(kBraveRewards, "BraveRewards", base::FEATURE_DISABLED_BY_DEFAULT);
#else
BASE_FEATURE(kBraveRewards, "BraveRewards", base::FEATURE_ENABLED_BY_DEFAULT);
#endif
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
BASE_FEATURE(kGeminiFeature,
             "BraveRewardsGemini",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

BASE_FEATURE(kVerboseLoggingFeature,
             "BraveRewardsVerboseLogging",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kAllowUnsupportedWalletProvidersFeature,
             "BraveRewardsAllowUnsupportedWalletProviders",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kAllowSelfCustodyProvidersFeature,
             "BraveRewardsAllowSelfCustodyProviders",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNewRewardsUIFeature,
             "BraveRewardsNewRewardsUI",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kAnimatedBackgroundFeature,
             "BraveRewardsAnimatedBackground",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kPlatformCreatorDetectionFeature,
             "BraveRewardsPlatformCreatorDetection",
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

}  // namespace brave_rewards::features
