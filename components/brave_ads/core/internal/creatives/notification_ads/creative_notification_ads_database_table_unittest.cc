/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/test/creative_notification_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeNotificationAdsDatabaseTableTest : public test::TestBase {
 protected:
  database::table::CreativeNotificationAds database_table_;
};

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, SaveEmpty) {
  // Act
  database::SaveCreativeNotificationAds({});

  // Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForActiveCampaigns(
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(segments, ::testing::IsEmpty());
  EXPECT_THAT(creative_ads, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, Save) {
  // Arrange
  const CreativeNotificationAdList saved_creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/2);

  // Act
  database::SaveCreativeNotificationAds(saved_creative_ads);

  // Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForActiveCampaigns(
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((SegmentList{"architecture", "arts & entertainment"}), segments);
  EXPECT_THAT(creative_ads,
              ::testing::UnorderedElementsAreArray(saved_creative_ads));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, SaveInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativeNotificationAdList saved_creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/3);

  // Act
  database::SaveCreativeNotificationAds(saved_creative_ads);

  // Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForActiveCampaigns(
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((SegmentList{"architecture", "arts & entertainment", "automotive"}),
            segments);
  EXPECT_THAT(creative_ads,
              ::testing::UnorderedElementsAreArray(saved_creative_ads));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, DoNotSaveDuplicates) {
  // Arrange
  const CreativeNotificationAdList saved_creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(saved_creative_ads);

  // Act
  database::SaveCreativeNotificationAds(saved_creative_ads);

  // Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForActiveCampaigns(
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(SegmentList{"architecture"}, segments);
  EXPECT_THAT(creative_ads,
              ::testing::UnorderedElementsAreArray(saved_creative_ads));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.segment = "food & drink";

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.segment = "technology & computing";

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  // Act & Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForSegments(
      /*segments=*/{"food & drink"},
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(SegmentList{"food & drink"}, segments);
  EXPECT_THAT(creative_ads, ::testing::ElementsAre(creative_ad_1));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForEmptySegments) {
  // Arrange
  const CreativeNotificationAdList saved_creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(saved_creative_ads);

  // Act & Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForSegments(
      /*segments=*/{},
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(segments, ::testing::IsEmpty());
  EXPECT_THAT(creative_ads, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetForNonExistentSegment) {
  // Arrange
  const CreativeNotificationAdList saved_creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(saved_creative_ads);

  // Act & Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForSegments(
      /*segments=*/{"NON_EXISTENT"},
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(SegmentList{"NON_EXISTENT"}, segments);
  EXPECT_THAT(creative_ads, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetForMultipleSegments) {
  // Arrange
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_3.segment = "automotive";

  database::SaveCreativeNotificationAds(
      {creative_ad_1, creative_ad_2, creative_ad_3});

  // Act & Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForSegments(
      /*segments=*/{"technology & computing", "food & drink"},
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((SegmentList{"technology & computing", "food & drink"}), segments);
  EXPECT_THAT(creative_ads,
              ::testing::UnorderedElementsAre(creative_ad_1, creative_ad_2));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetNonExpired) {
  // Arrange
  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_1.start_at = test::DistantPast();
  creative_ad_1.end_at = test::Now();

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  creative_ad_2.start_at = test::DistantPast();
  creative_ad_2.end_at = test::DistantFuture();

  database::SaveCreativeNotificationAds({creative_ad_1, creative_ad_2});

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
      test_future;
  database_table_.GetForActiveCampaigns(
      test_future
          .GetCallback<bool, const SegmentList&, CreativeNotificationAdList>());
  const auto [success, segments, creative_ads] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(SegmentList{creative_ad_2.segment}, segments);
  EXPECT_THAT(creative_ads, ::testing::ElementsAre(creative_ad_2));
}

}  // namespace brave_ads
