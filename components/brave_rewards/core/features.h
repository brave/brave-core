/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "build/build_config.h"

namespace brave_rewards::features {

#if BUILDFLAG(IS_ANDROID)
BASE_DECLARE_FEATURE(kBraveRewards);
#endif  // BUILDFLAG(IS_ANDROID)

BASE_DECLARE_FEATURE(kVerboseLoggingFeature);

BASE_DECLARE_FEATURE(kAllowUnsupportedWalletProvidersFeature);

BASE_DECLARE_FEATURE(kAllowSelfCustodyProvidersFeature);

BASE_DECLARE_FEATURE(kAnimatedBackgroundFeature);

BASE_DECLARE_FEATURE(kOfferEventsFeature);

// Griffin params for offer events.
// An https URL, or a localhost URL for local testing. Defaults to the Rewards
// API host.
inline constexpr base::FeatureParam<std::string> kOfferEventsServiceUrl{
    &kOfferEventsFeature, "service_url", ""};
// Issuer public keys the browser accepts, as "type:KEY,KEY;type:KEY".
inline constexpr base::FeatureParam<std::string> kOfferEventsPinnedKeys{
    &kOfferEventsFeature, "pinned_keys", ""};
inline constexpr base::FeatureParam<base::TimeDelta> kOfferEventsMinSendDelay{
    &kOfferEventsFeature, "min_send_delay", base::Minutes(1)};
inline constexpr base::FeatureParam<base::TimeDelta> kOfferEventsMaxSendDelay{
    &kOfferEventsFeature, "max_send_delay", base::Minutes(10)};

BASE_DECLARE_FEATURE(kPlatformCreatorDetectionFeature);

}  // namespace brave_rewards::features

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_FEATURES_H_
