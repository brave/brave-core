/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/priority/priority.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPriorityTest : public test::TestBase {};

TEST_F(BraveAdsPriorityTest,
       SortCreativeAdsIntoBucketsByPriorityForNoCreativeAds) {
  // Act
  const PrioritizedCreativeAdBuckets<CreativeAdList>
      prioritized_creative_ad_buckets =
          SortCreativeAdsIntoBucketsByPriority(CreativeAdList{});

  // Assert
  EXPECT_THAT(prioritized_creative_ad_buckets, ::testing::IsEmpty());
}

TEST_F(BraveAdsPriorityTest,
       SortCreativeAdsIntoBucketsByPriorityForMultipleCreativeAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.priority = 1;
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.priority = 2;
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_3.priority = 1;
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_4.priority = 3;
  creative_ads.push_back(creative_ad_4);

  CreativeNotificationAdInfo creative_ad_5 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_5.priority = 0;
  creative_ads.push_back(creative_ad_5);

  // Act
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      prioritized_creative_ad_buckets =
          SortCreativeAdsIntoBucketsByPriority(creative_ads);

  // Assert
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      expected_prioritized_creative_ad_buckets = {
          {/*priority=*/1, {creative_ad_1, creative_ad_3}},
          {/*priority=*/2, {creative_ad_2}},
          {/*priority=*/3, {creative_ad_4}}};
  EXPECT_THAT(expected_prioritized_creative_ad_buckets,
              ::testing::ElementsAreArray(prioritized_creative_ad_buckets));
}

TEST_F(BraveAdsPriorityTest,
       SortCreativeAdsIntoBucketsByPriorityForSingleCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.priority = 3;
  creative_ads.push_back(creative_ad_1);

  // Act
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      prioritized_creative_ad_buckets =
          SortCreativeAdsIntoBucketsByPriority(creative_ads);

  // Assert
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      expected_prioritized_creative_ad_buckets = {
          {/*priority*/ 3, {creative_ad_1}}};
  EXPECT_THAT(expected_prioritized_creative_ad_buckets,
              prioritized_creative_ad_buckets);
}

TEST_F(BraveAdsPriorityTest,
       DoNotSortCreativeAdsIntoBucketsForZeroPriorityCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.priority = 0;
  creative_ads.push_back(creative_ad_1);

  // Act
  const PrioritizedCreativeAdBuckets<CreativeNotificationAdList>
      prioritized_creative_ad_buckets =
          SortCreativeAdsIntoBucketsByPriority(creative_ads);

  // Assert
  EXPECT_THAT(prioritized_creative_ad_buckets, ::testing::IsEmpty());
}

}  // namespace brave_ads
