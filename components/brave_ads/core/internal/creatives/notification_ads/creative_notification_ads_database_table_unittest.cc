/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeNotificationAdsDatabaseTableTest : public UnitTestBase {
 protected:
  database::table::CreativeNotificationAds database_table_;
};

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, SaveEmpty) {
  // Act
  database::SaveCreativeNotificationAds({});

  // Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, Save) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/2);

  // Act
  database::SaveCreativeNotificationAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            SegmentList{"architecture", "arts & entertainment"},
                            testing::UnorderedElementsAreArray(creative_ads)));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, SaveInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/3);

  // Act
  database::SaveCreativeNotificationAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            SegmentList{"architecture", "arts & entertainment",
                                        "automotive"},
                            testing::UnorderedElementsAreArray(creative_ads)));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, DoNotSaveDuplicates) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act
  database::SaveCreativeNotificationAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{"architecture"}, creative_ads));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "food & drink";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "technology & computing";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"food & drink"},
                            CreativeNotificationAdList{creative_ad_1}));
  database_table_.GetForSegments(/*segments=*/{"food & drink"}, callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForEmptySegments) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()));
  database_table_.GetForSegments(/*segments=*/{}, callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetForNonExistentSegment) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"NON_EXISTENT"},
                            /*creative_ads=*/::testing::IsEmpty()));
  database_table_.GetForSegments(
      /*segments=*/{"NON_EXISTENT"}, callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetForMultipleSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.segment = "automotive";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          SegmentList{"technology & computing", "food & drink"},
          ::testing::UnorderedElementsAreArray(
              CreativeNotificationAdList{creative_ad_1, creative_ad_2})));
  database_table_.GetForSegments(
      /*segments=*/{"technology & computing", "food & drink"}, callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetNonExpired) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.start_at = DistantPast();
  creative_ad_1.end_at = Now();
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.start_at = DistantPast();
  creative_ad_2.end_at = DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{creative_ad_2.segment},
                  CreativeNotificationAdList{creative_ad_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("creative_ad_notifications", database_table_.GetTableName());
}

}  // namespace brave_ads
