/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsCreativeInlineContentAdsDatabaseTableTest : public UnitTestBase {
 protected:
  CreativeInlineContentAds database_table_;
};

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       SaveEmptyCreativeInlineContentAds) {
  // Arrange

  // Act
  database::SaveCreativeInlineContentAds({});

  // Assert
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       SaveCreativeInlineContentAds) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      BuildCreativeInlineContentAdsForTesting(/*count*/ 2);

  // Act
  database::SaveCreativeInlineContentAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeInlineContentAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       SaveCreativeInlineContentAdsInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativeInlineContentAdList creative_ads =
      BuildCreativeInlineContentAdsForTesting(/*count*/ 3);

  // Act
  database::SaveCreativeInlineContentAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeInlineContentAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       DoNotSaveDuplicateCreativeInlineContentAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ads.push_back(creative_ad);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  database::SaveCreativeInlineContentAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeInlineContentAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForSegmentsAndDimensions) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "food & drink";
  creative_ad_1.dimensions = "200x100";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "technology & computing-software";
  creative_ad_2.dimensions = "300x200";
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ad_3.dimensions = "150x150";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_1};

  database_table_.GetForSegmentsAndDimensions(
      /*segments*/ {"food & drink"}, /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeInlineContentAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForCreativeInstanceId) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  const CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ads.push_back(creative_ad_1);

  const CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForCreativeInstanceId(
      creative_ad_1.creative_instance_id,
      base::BindOnce(
          [](const CreativeInlineContentAdInfo& expected_creative_ad,
             const bool success, const std::string& /*creative_instance_id*/,
             const CreativeInlineContentAdInfo& creative_ad) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_creative_ad, creative_ad);
          },
          creative_ad_1));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForNonExistentCreativeInstanceId) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      BuildCreativeInlineContentAdsForTesting(/*count*/ 1);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForCreativeInstanceId(
      kMissingCreativeInstanceId,
      base::BindOnce([](const bool success,
                        const std::string& /*creative_instance_id*/,
                        const CreativeInlineContentAdInfo& /*creative_ad*/) {
        EXPECT_FALSE(success);
      }));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForEmptySegments) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      BuildCreativeInlineContentAdsForTesting(/*count*/ 1);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForSegmentsAndDimensions(
      /*segments*/ {}, /*dimensions*/ "200x100",
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeInlineContentAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForNonExistentCategory) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      BuildCreativeInlineContentAdsForTesting(/*count*/ 1);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForSegmentsAndDimensions(
      /*segments*/ {"FOOBAR"}, /*dimensions*/ "200x100",
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeInlineContentAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsFromMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeInlineContentAdInfo creative_ad_3 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "automobiles";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_1,
                                                       creative_ad_2};

  database_table_.GetForSegmentsAndDimensions(
      /*segments*/ {creative_ad_1.segment, creative_ad_2.segment},
      /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeInlineContentAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetNonExpiredCreativeInlineContentAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.start_at = DistantPast();
  creative_ad_1.end_at = Now();
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.start_at = DistantPast();
  creative_ad_2.end_at = DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act
  AdvanceClockBy(base::Hours(1));

  // Assert
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  database_table_.GetAll(base::BindOnce(
      [](const CreativeInlineContentAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      },
      std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsMatchingCaseInsensitiveSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeInlineContentAdInfo creative_ad_1 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeInlineContentAdInfo creative_ad_2 =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeInlineContentAds(creative_ads);

  // Act

  // Assert
  CreativeInlineContentAdList expected_creative_ads = {creative_ad_2};

  database_table_.GetForSegmentsAndDimensions(
      /*segments*/ {"FoOd & DrInK"}, /*dimensions*/ "200x100",
      base::BindOnce(
          [](const CreativeInlineContentAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeInlineContentAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeInlineContentAdsDatabaseTableTest, TableName) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("creative_inline_content_ads", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
