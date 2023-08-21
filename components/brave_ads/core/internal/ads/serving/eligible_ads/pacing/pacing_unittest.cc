/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pacing/pacing_random_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

std::vector<double> GetPacingRandomNumbers() {
  return std::vector<double>{0.0, 0.5, 0.99};
}

}  // namespace

class BraveAdsPacingTest : public UnitTestBase {};

TEST_F(BraveAdsPacingTest, PaceCreativeAdsWithMinPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.pass_through_rate = 0.0;
  creative_ads.push_back(creative_ad);

  // Act
  for (const double number : GetPacingRandomNumbers()) {
    const ScopedPacingRandomNumberSetterForTesting scoped_setter(number);
    const CreativeNotificationAdList paced_creative_ads =
        PaceCreativeAds(creative_ads);

    // Assert
    EXPECT_TRUE(paced_creative_ads.empty());
  }
}

TEST_F(BraveAdsPacingTest, DoNotPaceCreativeAdsWithMaxPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.pass_through_rate = 1.0;
  creative_ads.push_back(creative_ad);

  // Act
  for (const double number : GetPacingRandomNumbers()) {
    const ScopedPacingRandomNumberSetterForTesting scoped_setter(number);
    const CreativeNotificationAdList paced_creative_ads =
        PaceCreativeAds(creative_ads);

    // Assert
    const CreativeNotificationAdList expected_paced_creative_ads = {
        creative_ad};
    EXPECT_EQ(expected_paced_creative_ads, paced_creative_ads);
  }
}

TEST_F(BraveAdsPacingTest,
       PaceCreativeAdIfPacingIsGreaterThanOrEqualToPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad.pass_through_rate = 0.5;
  creative_ads.push_back(creative_ad);

  // Act
  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.7);
  const CreativeNotificationAdList paced_creative_ads =
      PaceCreativeAds(creative_ads);

  // Assert
  EXPECT_TRUE(paced_creative_ads.empty());
}

TEST_F(BraveAdsPacingTest,
       DoNotPaceCreativeAdWhenPacingIsLessThanPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.pass_through_rate = 0.1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.pass_through_rate = 0.5;
  creative_ads.push_back(creative_ad_2);

  // Act
  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.3);
  const CreativeNotificationAdList paced_creative_ads =
      PaceCreativeAds(creative_ads);

  // Assert
  const CreativeNotificationAdList expected_paced_creative_ads = {
      creative_ad_2};
  EXPECT_EQ(expected_paced_creative_ads, paced_creative_ads);
}

}  // namespace brave_ads
