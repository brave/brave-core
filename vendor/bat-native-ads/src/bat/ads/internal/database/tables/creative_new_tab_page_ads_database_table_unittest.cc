/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"

#include "bat/ads/internal/bundle/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeNewTabPageAdsDatabaseTableTest : public UnitTestBase {
 protected:
  BatAdsCreativeNewTabPageAdsDatabaseTableTest() = default;

  ~BatAdsCreativeNewTabPageAdsDatabaseTableTest() override = default;

  database::table::CreativeNewTabPageAds database_table;
};

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       SaveEmptyCreativeNewTabPageAds) {
  // Arrange

  // Act
  SaveCreativeAds({});

  // Assert
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       SaveCreativeNewTabPageAds) {
  // Arrange
  const CreativeNewTabPageAdList& creative_ads = BuildCreativeNewTabPageAds(2);

  // Act
  SaveCreativeAds(creative_ads);

  // Assert
  const CreativeNewTabPageAdList& expected_creative_ads = creative_ads;

  database_table.GetAll(
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       SaveCreativeNewTabPageAdsInBatches) {
  // Arrange
  database_table.set_batch_size(2);

  const CreativeNewTabPageAdList& creative_ads = BuildCreativeNewTabPageAds(3);

  // Act
  SaveCreativeAds(creative_ads);

  // Assert
  const CreativeNewTabPageAdList& expected_creative_ads = creative_ads;

  database_table.GetAll(
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       DoNotSaveDuplicateCreativeNewTabPageAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  creative_ads.push_back(creative_ad);

  SaveCreativeAds(creative_ads);

  // Act
  SaveCreativeAds(creative_ads);

  // Assert
  CreativeNewTabPageAdList expected_creative_ads;
  expected_creative_ads.push_back(creative_ad);

  database_table.GetAll(
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest, GetForSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "food & drink";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_2);

  CreativeNewTabPageAdInfo creative_ad_3 = BuildCreativeNewTabPageAd();
  creative_ad_3.segment = "food & drink";
  creative_ads.push_back(creative_ad_3);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  CreativeNewTabPageAdList expected_creative_ads;
  expected_creative_ads.push_back(creative_ad_1);
  expected_creative_ads.push_back(creative_ad_3);

  const SegmentList& segments = {"food & drink"};

  database_table.GetForSegments(
      segments,
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForCreativeInstanceId) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  const CreativeNewTabPageAdInfo& creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ads.push_back(creative_ad_1);

  const CreativeNewTabPageAdInfo& creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  const CreativeNewTabPageAdInfo& expected_creative_ad = creative_ad_1;

  database_table.GetForCreativeInstanceId(
      expected_creative_ad.creative_instance_id,
      [&expected_creative_ad](const bool success,
                              const std::string& creative_instance_id,
                              const CreativeNewTabPageAdInfo& creative_ad) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ad, creative_ad);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForNonExistentCreativeInstanceId) {
  // Arrange
  const CreativeNewTabPageAdList& creative_ads = BuildCreativeNewTabPageAds(1);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  database_table.GetForCreativeInstanceId(
      "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
      [](const bool success, const std::string& creative_instance_id,
         const CreativeNewTabPageAdInfo& creative_ad) {
        EXPECT_FALSE(success);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForEmptySegments) {
  // Arrange
  const CreativeNewTabPageAdList& creative_ads = BuildCreativeNewTabPageAds(1);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  const SegmentList& segments = {""};

  database_table.GetForSegments(
      segments, [](const bool success, const SegmentList& segments,
                   const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsForNonExistentSegment) {
  // Arrange
  const CreativeNewTabPageAdList& creative_ads = BuildCreativeNewTabPageAds(1);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  const SegmentList& segments = {"FOOBAR"};

  database_table.GetForSegments(
      segments, [](const bool success, const SegmentList& segments,
                   const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(creative_ads.empty());
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsFromMultipleSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  CreativeNewTabPageAdInfo creative_ad_3 = BuildCreativeNewTabPageAd();
  creative_ad_3.segment = "automobiles";
  creative_ads.push_back(creative_ad_3);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  CreativeNewTabPageAdList expected_creative_ads;
  expected_creative_ads.push_back(creative_ad_1);
  expected_creative_ads.push_back(creative_ad_2);

  const SegmentList& segments = {creative_ad_1.segment, creative_ad_2.segment};

  database_table.GetForSegments(
      segments,
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetNonExpiredCreativeNewTabPageAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.start_at = DistantPast();
  creative_ad_1.end_at = Now();
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.start_at = DistantPast();
  creative_ad_2.end_at = DistantFuture();
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  FastForwardClockBy(base::Seconds(1));

  // Assert
  CreativeNewTabPageAdList expected_creative_ads;
  expected_creative_ads.push_back(creative_ad_2);

  database_table.GetAll(
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest,
       GetCreativeNewTabPageAdsMatchingCaseInsensitiveSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 = BuildCreativeNewTabPageAd();
  creative_ad_1.segment = "technology & computing-software";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 = BuildCreativeNewTabPageAd();
  creative_ad_2.segment = "food & drink";
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act

  // Assert
  CreativeNewTabPageAdList expected_creative_ads;
  expected_creative_ads.push_back(creative_ad_2);

  const SegmentList& segments = {"FoOd & DrInK"};

  database_table.GetForSegments(
      segments,
      [&expected_creative_ads](const bool success, const SegmentList& segments,
                               const CreativeNewTabPageAdList& creative_ads) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ads, creative_ads);
      });
}

TEST_F(BatAdsCreativeNewTabPageAdsDatabaseTableTest, TableName) {
  // Arrange

  // Act
  const std::string& table_name = database_table.GetTableName();

  // Assert
  const std::string& expected_table_name = "creative_new_tab_page_ads";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
