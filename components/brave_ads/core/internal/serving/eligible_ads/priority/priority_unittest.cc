/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPriorityTest : public UnitTestBase {};

TEST_F(BraveAdsPriorityTest, DoNotPrioritizeIfNoCreativeAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads;

  // Act
  const CreativeNotificationAdList prioritized_creative_ads =
      HighestPriorityCreativeAds(creative_ads);

  // Assert
  EXPECT_TRUE(prioritized_creative_ads.empty());
}

TEST_F(BraveAdsPriorityTest, SortCreativeAdsIntoBucketsByPriority) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.priority = 2;
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.priority = 1;
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_4.priority = 3;
  creative_ads.push_back(creative_ad_4);

  CreativeNotificationAdInfo creative_ad_5 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_5.priority = 0;
  creative_ads.push_back(creative_ad_5);

  // Act & Assert
  const base::flat_map</*priority*/ int, CreativeNotificationAdList>
      expected_prioritized_creative_ads = {{1, {creative_ad_1, creative_ad_3}},
                                           {2, {creative_ad_2}},
                                           {3, {creative_ad_4}}};
  EXPECT_THAT(expected_prioritized_creative_ads,
              ::testing::ElementsAreArray(
                  SortCreativeAdsIntoBucketsByPriority(creative_ads)));
}

TEST_F(BraveAdsPriorityTest, HighestPriorityCreativeAdsForSingleCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.priority = 1;
  creative_ads.push_back(creative_ad);

  // Act & Assert
  const CreativeNotificationAdList expected_prioritized_creative_ads = {
      creative_ad};
  EXPECT_EQ(expected_prioritized_creative_ads,
            HighestPriorityCreativeAds(creative_ads));
}

TEST_F(BraveAdsPriorityTest, HighestPriorityCreativeAdsForMultipleCreativeAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.priority = 2;
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.priority = 1;
  creative_ads.push_back(creative_ad_3);

  // Act & Assert
  const CreativeNotificationAdList expected_prioritized_creative_ads = {
      creative_ad_1, creative_ad_3};
  EXPECT_THAT(
      expected_prioritized_creative_ads,
      ::testing::ElementsAreArray(HighestPriorityCreativeAds(creative_ads)));
}

TEST_F(BraveAdsPriorityTest, DoNotPrioritizeZeroPriorityCreativeAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.priority = 0;
  creative_ads.push_back(creative_ad_2);

  // Act & Assert
  const CreativeNotificationAdList expected_prioritized_creative_ads = {
      creative_ad_1};
  EXPECT_EQ(expected_prioritized_creative_ads,
            HighestPriorityCreativeAds(creative_ads));
}

}  // namespace brave_ads
