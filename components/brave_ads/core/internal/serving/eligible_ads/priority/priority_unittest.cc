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

TEST_F(BraveAdsPriorityTest,
       SortCreativeAdsIntoBucketsByPriorityForNoCreativeAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads;

  // Act & Assert
  EXPECT_THAT(SortCreativeAdsIntoBucketsByPriority(creative_ads),
              ::testing::IsEmpty());
}

TEST_F(BraveAdsPriorityTest,
       SortCreativeAdsIntoBucketsByPriorityForMultipleCreativeAds) {
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
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      expected_buckets = {{1, {creative_ad_1, creative_ad_3}},
                          {2, {creative_ad_2}},
                          {3, {creative_ad_4}}};
  EXPECT_THAT(expected_buckets,
              ::testing::ElementsAreArray(
                  SortCreativeAdsIntoBucketsByPriority(creative_ads)));
}

TEST_F(BraveAdsPriorityTest,
       SortCreativeAdsIntoBucketsByPriorityForSingleCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.priority = 3;
  creative_ads.push_back(creative_ad_1);

  // Act & Assert
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      expected_buckets = {{3, {creative_ad_1}}};
  EXPECT_THAT(expected_buckets,
              SortCreativeAdsIntoBucketsByPriority(creative_ads));
}

TEST_F(BraveAdsPriorityTest,
       DoNotSortCreativeAdsIntoBucketsForZeroPriorityCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.priority = 0;
  creative_ads.push_back(creative_ad_1);

  // Act & Assert
  EXPECT_THAT(SortCreativeAdsIntoBucketsByPriority(creative_ads),
              ::testing::IsEmpty());
}

}  // namespace brave_ads
