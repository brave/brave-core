/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing.h"

#include <vector>

#include "bat/ads/internal/ads/serving/eligible_ads/pacing/pacing_random_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::vector<double> GetPacingRandomNumbers() {
  return std::vector<double>{0.0, 0.5, 0.99};
}

}  // namespace

TEST(BatAdsPacingTest, PaceCreativeAdsWithMinPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.ptr = 0.0;
  creative_ads.push_back(creative_ad);

  // Act
  for (const double number : GetPacingRandomNumbers()) {
    const ScopedPacingRandomNumberSetter scoped_setter(number);
    const CreativeNotificationAdList paced_creative_ads =
        PaceCreativeAds(creative_ads);

    // Assert
    EXPECT_TRUE(paced_creative_ads.empty());
  }
}

TEST(BatAdsPacingTest, DoNotPaceCreativeAdsWithMaxPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.ptr = 1.0;
  creative_ads.push_back(creative_ad);

  // Act
  for (const double number : GetPacingRandomNumbers()) {
    const ScopedPacingRandomNumberSetter scoped_setter(number);
    const CreativeNotificationAdList paced_creative_ads =
        PaceCreativeAds(creative_ads);

    // Assert
    const CreativeNotificationAdList expected_paced_creative_ads = {
        creative_ad};
    EXPECT_EQ(expected_paced_creative_ads, paced_creative_ads);
  }
}

TEST(BatAdsPacingTest,
     PaceCreativeAdIfPacingIsGreaterThanOrEqualToPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.ptr = 0.5;
  creative_ads.push_back(creative_ad);

  // Act
  const ScopedPacingRandomNumberSetter scoped_setter(0.7);
  const CreativeNotificationAdList paced_creative_ads =
      PaceCreativeAds(creative_ads);

  // Assert
  EXPECT_TRUE(paced_creative_ads.empty());
}

TEST(BatAdsPacingTest, DoNotPaceCreativeAdWhenPacingIsLessThanPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.ptr = 0.1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.ptr = 0.5;
  creative_ads.push_back(creative_ad_2);

  // Act
  const ScopedPacingRandomNumberSetter scoped_setter(0.3);
  const CreativeNotificationAdList paced_creative_ads =
      PaceCreativeAds(creative_ads);

  // Assert
  const CreativeNotificationAdList expected_paced_creative_ads = {
      creative_ad_2};
  EXPECT_EQ(expected_paced_creative_ads, paced_creative_ads);
}

}  // namespace ads
