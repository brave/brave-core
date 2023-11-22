/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing_random_util.h"

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
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.pass_through_rate = 0.0;
  creative_ads.push_back(creative_ad);

  // Act & Assert
  for (const double number : GetPacingRandomNumbers()) {
    const ScopedPacingRandomNumberSetterForTesting scoped_setter(number);
    const CreativeNotificationAdList paced_creative_ads =
        PaceCreativeAds(creative_ads);
    EXPECT_TRUE(paced_creative_ads.empty());
  }
}

TEST_F(BraveAdsPacingTest, DoNotPaceCreativeAdsWithMaxPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.pass_through_rate = 1.0;
  creative_ads.push_back(creative_ad);

  // Act & Assert
  const CreativeNotificationAdList expected_paced_creative_ads = {creative_ad};
  for (const double number : GetPacingRandomNumbers()) {
    const ScopedPacingRandomNumberSetterForTesting scoped_setter(number);
    EXPECT_EQ(expected_paced_creative_ads, PaceCreativeAds(creative_ads));
  }
}

TEST_F(BraveAdsPacingTest,
       PaceCreativeAdIfPacingIsGreaterThanOrEqualToPassThroughRate) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.pass_through_rate = 0.5;
  creative_ads.push_back(creative_ad);

  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.7);

  // Act
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
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.pass_through_rate = 0.1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.pass_through_rate = 0.5;
  creative_ads.push_back(creative_ad_2);

  const ScopedPacingRandomNumberSetterForTesting scoped_setter(0.3);

  // Act & Assert
  const CreativeNotificationAdList expected_paced_creative_ads = {
      creative_ad_2};
  EXPECT_EQ(expected_paced_creative_ads, PaceCreativeAds(creative_ads));
}

}  // namespace brave_ads
