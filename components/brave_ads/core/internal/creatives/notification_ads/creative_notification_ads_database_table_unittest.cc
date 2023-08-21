/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsCreativeNotificationAdsDatabaseTableTest : public UnitTestBase {
 protected:
  CreativeNotificationAds database_table_;
};

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       SaveEmptyCreativeNotificationAds) {
  // Arrange

  // Act
  database::SaveCreativeNotificationAds({});

  // Assert
  database_table_.GetAll(
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNotificationAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       SaveCreativeNotificationAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      BuildCreativeNotificationAdsForTesting(/*count*/ 2);

  // Act
  database::SaveCreativeNotificationAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeNotificationAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       SaveCreativeNotificationAdsInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const CreativeNotificationAdList creative_ads =
      BuildCreativeNotificationAdsForTesting(/*count*/ 3);

  // Act
  database::SaveCreativeNotificationAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeNotificationAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       DoNotSaveDuplicateCreativeNotificationAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      BuildCreativeNotificationAdsForTesting(/*count*/ 1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act
  database::SaveCreativeNotificationAds(creative_ads);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const CreativeNotificationAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      },
      creative_ads));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "food & drink";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads = {creative_ad_1,
                                                      creative_ad_3};

  database_table_.GetForSegments(
      /*segments*/ {"food & drink"},
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsForEmptySegments) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      BuildCreativeNotificationAdsForTesting(/*count*/ 1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForSegments(
      /*segments*/ {},
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNotificationAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsForNonExistentSegment) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      BuildCreativeNotificationAdsForTesting(/*count*/ 1);
  database::SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  database_table_.GetForSegments(
      /*segments*/ {"FOOBAR"},
      base::BindOnce([](const bool success, const SegmentList& /*segments*/,
                        const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsFromMultipleSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "automobiles";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads = {creative_ad_1,
                                                      creative_ad_2};

  database_table_.GetForSegments(
      /*segments*/ {creative_ad_1.segment, creative_ad_2.segment},
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetNonExpiredCreativeNotificationAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.start_at = DistantPast();
  creative_ad_1.end_at = Now();
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.start_at = DistantPast();
  creative_ad_2.end_at = DistantFuture();
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act
  AdvanceClockBy(base::Hours(1));

  // Assert
  CreativeNotificationAdList expected_creative_ads = {creative_ad_2};

  database_table_.GetAll(base::BindOnce(
      [](const CreativeNotificationAdList& expected_creative_ads,
         const bool success, const SegmentList& /*segments*/,
         const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      },
      std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsMatchingCaseInsensitiveSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads = {creative_ad_2};

  database_table_.GetForSegments(
      /*segments*/ {"FoOd & DrInK"},
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_EQ(expected_creative_ads, creative_ads);
          },
          std::move(expected_creative_ads)));
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableTest, TableName) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("creative_ad_notifications", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
