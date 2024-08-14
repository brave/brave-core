/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeInlineContentAdsDatabaseTableTest
    : public test::TestBase {
 protected:
  database::table::CreativeInlineContentAds database_table_;
};

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, SaveEmpty) {
  // Act
  database::SaveCreativeInlineContentAds({});

  // Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()));
  database_table_.GetForActiveCampaigns(callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, Save) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      test::BuildCreativeInlineContentAds(/*count=*/2);

  // Act
  database::SaveCreativeInlineContentAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true, SegmentList{"architecture", "arts & entertainment"},
          ::testing::UnorderedElementsAreArray(creative_ads)));
  database_table_.GetForActiveCampaigns(callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, SaveInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativeInlineContentAdList creative_ads =
      test::BuildCreativeInlineContentAds(/*count=*/3);

  // Act
  database::SaveCreativeInlineContentAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          SegmentList{"architecture", "arts & entertainment", "automotive"},
          ::testing::UnorderedElementsAreArray(creative_ads)));
  database_table_.GetForActiveCampaigns(callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, DoNotSaveDuplicates) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      test::BuildCreativeInlineContentAds(/*count=*/1);
  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  database::SaveCreativeInlineContentAds(creative_ads);

  // Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{"architecture"}, creative_ads));
  database_table_.GetForActiveCampaigns(callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForSegmentsAndDimensions) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "food & drink";
  creative_ad_1.dimensions = "200x100";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "technology & computing";
  creative_ad_2.dimensions = "300x200";
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_3.segment = "food & drink";
  creative_ad_3.dimensions = "150x150";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"food & drink"},
                            CreativeInlineContentAdList{creative_ad_1}));
  database_table_.GetForSegmentsAndDimensions(SegmentList{"food & drink"},
                                              /*dimensions=*/"200x100",
                                              callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, GetForEmptySegments) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      test::BuildCreativeInlineContentAds(/*count=*/1);
  database::SaveCreativeInlineContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*segments=*/::testing::IsEmpty(),
                            /*creative_ads=*/::testing::IsEmpty()));
  database_table_.GetForSegmentsAndDimensions(
      /*segments=*/{}, /*dimensions=*/"200x100", callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForNonExistentSegment) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      test::BuildCreativeInlineContentAds(/*count=*/1);
  database::SaveCreativeInlineContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, SegmentList{"NON_EXISTENT"},
                            /*creative_ads=*/::testing::IsEmpty()));
  database_table_.GetForSegmentsAndDimensions(
      /*segments=*/{"NON_EXISTENT"}, /*dimensions=*/"200x100", callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "technology & computing";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_3.segment = "automotive";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true, SegmentList{"technology & computing", "automotive"},
          ::testing::UnorderedElementsAreArray(
              CreativeInlineContentAdList{creative_ad_1, creative_ad_3})));
  database_table_.GetForSegmentsAndDimensions(
      /*segments=*/{"technology & computing", "automotive"},
      /*dimensions=*/"200x100", callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForCreativeInstanceId) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  const CreativeInlineContentAdInfo creative_ad_1 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ads.push_back(creative_ad_1);

  const CreativeInlineContentAdInfo creative_ad_2 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            creative_ad_1.creative_instance_id, creative_ad_1));
  database_table_.GetForCreativeInstanceId(creative_ad_1.creative_instance_id,
                                           callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForNonExistentCreativeInstanceId) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ads.push_back(creative_ad);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/false, test::kMissingCreativeInstanceId,
                            CreativeInlineContentAdInfo{}));
  database_table_.GetForCreativeInstanceId(test::kMissingCreativeInstanceId,
                                           callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, GetNonExpired) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.start_at = test::DistantPast();
  creative_ad_1.end_at = test::Now();
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.start_at = test::DistantPast();
  creative_ad_2.end_at = test::DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  base::MockCallback<database::table::GetCreativeInlineContentAdsCallback>
      callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, SegmentList{creative_ad_2.segment},
                  CreativeInlineContentAdList{creative_ad_2}));
  database_table_.GetForActiveCampaigns(callback.Get());
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("creative_inline_content_ads", database_table_.GetTableName());
}

}  // namespace brave_ads
