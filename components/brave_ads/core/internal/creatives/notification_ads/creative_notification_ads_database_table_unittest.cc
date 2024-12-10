/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"

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
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
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
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            SegmentList{"architecture", "arts & entertainment"},
                            ::testing::UnorderedElementsAreArray(creative_ads)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
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
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            SegmentList{"architecture", "arts & entertainment",
                                        "automotive"},
                            ::testing::UnorderedElementsAreArray(creative_ads)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
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
  base::RunLoop run_loop;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{"architecture"}, creative_ads))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "food & drink";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "technology & computing";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"food & drink"},
                            CreativeNotificationAdList{creative_ad_1}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(/*segments=*/{"food & drink"}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForEmptySegments) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(/*segments=*/{}, callback.Get());
  run_loop.Run();
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
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"NON_EXISTENT"},
                            /*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(
      /*segments=*/{"NON_EXISTENT"}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetForMultipleSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_3.segment = "automotive";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          SegmentList{"technology & computing", "food & drink"},
          ::testing::UnorderedElementsAreArray(
              CreativeNotificationAdList{creative_ad_1, creative_ad_2})))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(
      /*segments=*/{"technology & computing", "food & drink"}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetNonExpired) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.start_at = test::DistantPast();
  creative_ad_1.end_at = test::Now();
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.start_at = test::DistantPast();
  creative_ad_2.end_at = test::DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  base::MockCallback<database::table::GetCreativeNotificationAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{creative_ad_2.segment},
                  CreativeNotificationAdList{creative_ad_2}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("creative_ad_notifications", database_table_.GetTableName());
}

}  // namespace brave_ads
