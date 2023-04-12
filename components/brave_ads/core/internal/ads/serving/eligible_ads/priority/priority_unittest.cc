/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/priority/priority.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsPriorityTest, PrioritizeNoCreativeAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads;

  // Act
  const CreativeNotificationAdList prioritized_creative_ads =
      PrioritizeCreativeAds(creative_ads);

  // Assert
  EXPECT_TRUE(prioritized_creative_ads.empty());
}

TEST(BatAdsPriorityTest, PrioritizeSingleCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad.priority = 1;
  creative_ads.push_back(creative_ad);

  // Act
  const CreativeNotificationAdList prioritized_creative_ads =
      PrioritizeCreativeAds(creative_ads);

  // Assert
  const CreativeNotificationAdList expected_prioritized_creative_ads = {
      creative_ad};
  EXPECT_EQ(expected_prioritized_creative_ads, prioritized_creative_ads);
}

TEST(BatAdsPriorityTest, PrioritizeMultipleCreativeAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_2.priority = 2;
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_3.priority = 1;
  creative_ads.push_back(creative_ad_3);

  // Act
  const CreativeNotificationAdList prioritized_creative_ads =
      PrioritizeCreativeAds(creative_ads);

  // Assert
  const CreativeNotificationAdList expected_prioritized_creative_ads = {
      creative_ad_1, creative_ad_3};
  EXPECT_TRUE(base::ranges::equal(expected_prioritized_creative_ads,
                                  prioritized_creative_ads));
}

TEST(BatAdsPriorityTest, DoNotPrioritizeZeroPriorityCreativeAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_2.priority = 0;
  creative_ads.push_back(creative_ad_2);

  // Act
  const CreativeNotificationAdList prioritized_creative_ads =
      PrioritizeCreativeAds(creative_ads);

  // Assert
  const CreativeNotificationAdList expected_prioritized_creative_ads = {
      creative_ad_1};
  EXPECT_EQ(expected_prioritized_creative_ads, prioritized_creative_ads);
}

}  // namespace brave_ads
