/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_CONSTANTS_H_

#include <cstdint>

#include "build/build_config.h"

namespace brave_ads {

constexpr char kNotificationAdPlacementIdKey[] = "uuid";
constexpr char kNotificationAdCreativeInstanceIdKey[] = "creative_instance_id";
constexpr char kNotificationAdCreativeSetIdKey[] = "creative_set_id";
constexpr char kNotificationAdCampaignIdKey[] = "campaign_id";
constexpr char kNotificationAdAdvertiserIdKey[] = "advertiser_id";
constexpr char kNotificationAdSegmentKey[] = "segment";
constexpr char kNotificationAdTitleKey[] = "title";
constexpr char kNotificationAdBodyKey[] = "body";
constexpr char kNotificationAdTargetUrlKey[] = "target_url";

// Brave Ads per hour are user configurable within the brave://rewards ads UI.
#if !BUILDFLAG(IS_IOS)
constexpr int64_t kDefaultBraveRewardsNotificationAdsPerHour = 10;
#else
constexpr int64_t kDefaultBraveRewardsNotificationAdsPerHour = 2;
#endif  // !BUILDFLAG(IS_IOS)

// Default ad notification timeout in seconds.
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
constexpr int kDefaultNotificationAdTimeout = 120;
#else
constexpr int kDefaultNotificationAdTimeout = 30;
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

// Do not fallback to custom notification ad by default
constexpr bool kDefaultCanFallbackToCustomNotificationAds = false;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_CONSTANTS_H_
