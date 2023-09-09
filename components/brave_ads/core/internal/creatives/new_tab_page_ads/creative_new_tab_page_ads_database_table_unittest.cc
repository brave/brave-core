/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsCreativeNewTabPageAdsDatabaseTableTest : public UnitTestBase {
 protected:
  CreativeNewTabPageAds database_table_;
};

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       SaveEmptyCreativeNewTabPageAds) {
  // Arrange

  // Act
  database::SaveCreativeNewTabPageAds({});

  // Assert
  database_table_.GetAll(
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       SaveCreativeNewTabPageAds) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      BuildCreativeNewTabPageAdsForTesting(/*count*/ 2);

  // Act
  database::SaveCreativeNewTabPageAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeNewTabPageAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       SaveCreativeNewTabPageAdsInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativeNewTabPageAdList creative_ads =
      BuildCreativeNewTabPageAdsForTesting(/*count*/ 3);

  // Act
  database::SaveCreativeNewTabPageAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeNewTabPageAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       DoNotSaveDuplicateCreativeNewTabPageAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ads.push_back(creative_ad);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act
  database::SaveCreativeNewTabPageAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeNewTabPageAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "food & drink";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  CreativeNewTabPageAdInfo creative_ad_3 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  CreativeNewTabPageAdList expected_creative_ads = {creative_ad_1,
                                                    creative_ad_3};

  database_table_.GetForSegments(
      /*segments*/ {"food & drink"},
      base::BindOnce(
          [](const CreativeNewTabPageAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNewTabPageAdList& creative_ads) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForCreativeInstanceId) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  const CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ads.push_back(creative_ad_1);

  const CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForCreativeInstanceId(
      creative_ad_1.creative_instance_id,
      base::BindOnce(
          [](const CreativeNewTabPageAdInfo& expected_creative_ad,
             const bool success, const std::string& /*creative_instance_id*/,
             const CreativeNewTabPageAdInfo& creative_ad) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_creative_ad, creative_ad);
          },
          creative_ad_1));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForNonExistentCreativeInstanceId) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      BuildCreativeNewTabPageAdsForTesting(/*count*/ 1);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForCreativeInstanceId(
      kMissingCreativeInstanceId,
      base::BindOnce([](const bool success,
                        const std::string& /*creative_instance_id*/,
                        const CreativeNewTabPageAdInfo& /*creative_ad*/) {
        EXPECT_FALSE(success);
      }));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForEmptySegments) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      BuildCreativeNewTabPageAdsForTesting(/*count*/ 1);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForSegments(
      /*segments*/ {},
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForNonExistentSegment) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      BuildCreativeNewTabPageAdsForTesting(/*count*/ 1);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForSegments(
      /*segments*/ {"FOOBAR"},
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsFromMultipleSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeNewTabPageAdInfo creative_ad_3 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "automobiles";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  CreativeNewTabPageAdList expected_creative_ads = {creative_ad_1,
                                                    creative_ad_2};

  database_table_.GetForSegments(
      /*segments*/ {creative_ad_1.segment, creative_ad_2.segment},
      base::BindOnce(
          [](const CreativeNewTabPageAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNewTabPageAdList& creative_ads) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetNonExpiredCreativeNewTabPageAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.start_at = DistantPast();
  creative_ad_1.end_at = Now();
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.start_at = DistantPast();
  creative_ad_2.end_at = DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act
  AdvanceClockBy(base::Milliseconds(1));

  // Assert
  CreativeNewTabPageAdList expected_creative_ads = {creative_ad_2};

  database_table_.GetAll(base::BindOnce(
      [](const CreativeNewTabPageAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      },
      std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsMatchingCaseInsensitiveSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act

  // Assert
  CreativeNewTabPageAdList expected_creative_ads = {creative_ad_2};

  database_table_.GetForSegments(
      /*segments*/ {"FoOd & DrInK"},
      base::BindOnce(
          [](const CreativeNewTabPageAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNewTabPageAdList& creative_ads) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableTest, TableName) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("creative_new_tab_page_ads", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
