/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_TEST_CONSTANTS_H_

#include "base/time/time.h"

namespace brave_ads::test {

// Creative ad.
inline constexpr char kCreativeAdPlacementId[] =
    "953f362e-98cd-4fa6-8403-e886185b88fc";
inline constexpr char kCreativeAdPlacementIdWithUnreservedCharacters[] =
    "\"'+*&%";
inline constexpr char kEscapedCreativeAdPlacementIdWithUnreservedCharacters[] =
    "%22%27%2B%2A%26%25";
inline constexpr char kCreativeAdCreativeInstanceId[] =
    "d94f98e6-74f8-41cf-bfb2-3d753b94f8ef";
inline constexpr char kCreativeAdCreativeSetId[] =
    "1b3d1717-372b-4dfc-9aa5-f92cb3ac87ac";
inline constexpr char kCreativeAdCampaignId[] =
    "c2096fe1-3eb7-4096-8fca-04bca89eab35";
inline constexpr char kCreativeAdAdvertiserId[] =
    "c7bf8cbe-01bf-48a8-8282-57b5165c986a";
inline constexpr char kCreativeAdLandingPage[] = "https://brave.com";
inline constexpr char kCreativeAdHeadlineText[] = "Headline text";
inline constexpr char kCreativeAdDescription[] = "Description";
inline constexpr double kCreativeAdRewardsValue = 0.5;

// Creative set conversion.
inline constexpr char kCreativeSetConversionUrlPattern[] =
    "https://brave.com/*";
inline constexpr char kCreativeSetConversionAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
inline constexpr base::TimeDelta kCreativeSetConversionObservationWindow =
    base::Days(1);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_TEST_CONSTANTS_H_
