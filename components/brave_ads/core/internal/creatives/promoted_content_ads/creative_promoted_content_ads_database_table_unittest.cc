/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativePromotedContentAdsDatabaseTableTest
    : public test::TestBase {
 protected:
  database::table::CreativePromotedContentAds database_table_;
};

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest, SaveEmpty) {
  // Act
  database::SaveCreativePromotedContentAds({});

  // Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest, Save) {
  // Arrange
  const CreativePromotedContentAdList creative_ads =
      test::BuildCreativePromotedContentAds(/*count=*/2);

  // Act
  database::SaveCreativePromotedContentAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            SegmentList{"architecture", "arts & entertainment"},
                            ::testing::UnorderedElementsAreArray(creative_ads)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest, SaveInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativePromotedContentAdList creative_ads =
      test::BuildCreativePromotedContentAds(/*count=*/3);

  // Act
  database::SaveCreativePromotedContentAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
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

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest,
       DoNotSaveDuplicates) {
  // Arrange
  const CreativePromotedContentAdList creative_ads =
      test::BuildCreativePromotedContentAds(/*count=*/1);
  database::SaveCreativePromotedContentAds(creative_ads);

  // Act
  database::SaveCreativePromotedContentAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{"architecture"}, creative_ads))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativePromotedContentAdList creative_ads;

  CreativePromotedContentAdInfo creative_ad_1 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "food & drink";
  creative_ads.push_back(creative_ad_1);

  CreativePromotedContentAdInfo creative_ad_2 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "technology & computing";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativePromotedContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"food & drink"},
                            CreativePromotedContentAdList{creative_ad_1}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(
      /*segments=*/{"food & drink"}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest,
       GetForEmptySegments) {
  // Arrange
  const CreativePromotedContentAdList creative_ads =
      test::BuildCreativePromotedContentAds(/*count=*/1);
  database::SaveCreativePromotedContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(/*segments=*/{}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest,
       GetForNonExistentSegment) {
  // Arrange
  const CreativePromotedContentAdList creative_ads =
      test::BuildCreativePromotedContentAds(/*count=*/1);
  database::SaveCreativePromotedContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"NON_EXISTENT"},
                            /*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(
      /*segments=*/{"NON_EXISTENT"}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest,
       GetForMultipleSegments) {
  // Arrange
  CreativePromotedContentAdList creative_ads;

  CreativePromotedContentAdInfo creative_ad_1 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativePromotedContentAdInfo creative_ad_2 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativePromotedContentAdInfo creative_ad_3 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_3.segment = "automotive";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativePromotedContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          SegmentList{"technology & computing", "food & drink"},
          ::testing::UnorderedElementsAreArray(
              CreativePromotedContentAdList{creative_ad_1, creative_ad_2})))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForSegments(
      /*segments=*/{"technology & computing", "food & drink"}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest,
       GetForCreativeInstanceId) {
  // Arrange
  CreativePromotedContentAdList creative_ads;

  const CreativePromotedContentAdInfo creative_ad_1 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ads.push_back(creative_ad_1);

  const CreativePromotedContentAdInfo creative_ad_2 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativePromotedContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            creative_ad_1.creative_instance_id, creative_ad_1))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForCreativeInstanceId(creative_ad_1.creative_instance_id,
                                           callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest,
       GetForNonExistentCreativeInstanceId) {
  // Arrange
  const CreativePromotedContentAdList creative_ads =
      test::BuildCreativePromotedContentAds(/*count=*/1);
  database::SaveCreativePromotedContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/false, test::kMissingCreativeInstanceId,
                            CreativePromotedContentAdInfo{}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForCreativeInstanceId(test::kMissingCreativeInstanceId,
                                           callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest, GetNonExpired) {
  // Arrange
  CreativePromotedContentAdList creative_ads;

  CreativePromotedContentAdInfo creative_ad_1 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_1.start_at = test::DistantPast();
  creative_ad_1.end_at = test::Now();
  creative_ads.push_back(creative_ad_1);

  CreativePromotedContentAdInfo creative_ad_2 =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/true);
  creative_ad_2.start_at = test::DistantPast();
  creative_ad_2.end_at = test::DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativePromotedContentAds(creative_ads);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  base::MockCallback<database::table::GetCreativePromotedContentAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{creative_ad_2.segment},
                  CreativePromotedContentAdList{creative_ad_2}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database_table_.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativePromotedContentAdsDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("creative_promoted_content_ads", database_table_.GetTableName());
}

}  // namespace brave_ads
