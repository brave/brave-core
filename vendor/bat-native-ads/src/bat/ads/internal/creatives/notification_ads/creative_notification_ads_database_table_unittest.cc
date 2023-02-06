/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_container_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::database::table {

class BatAdsCreativeNotificationAdsDatabaseTableTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    database_table_ = std::make_unique<CreativeNotificationAds>();
  }

  std::unique_ptr<CreativeNotificationAds> database_table_;
};

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       SaveEmptyCreativeNotificationAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads;

  // Act
  SaveCreativeNotificationAds(creative_ads);

  // Assert
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       SaveCreativeNotificationAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.per_week = 4;
  info_1.per_month = 5;
  info_1.total_max = 6;
  info_1.value = 1.0;
  info_1.segment = "technology & computing-software";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = GURL("https://brave.com");
  info_1.title = "Test Ad 1 Title";
  info_1.body = "Test Ad 1 Body";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeNotificationAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 1;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 2;
  info_2.per_day = 3;
  info_2.per_week = 4;
  info_2.per_month = 5;
  info_2.total_max = 6;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = GURL("https://brave.com");
  info_2.title = "Test Ad 2 Title";
  info_2.body = "Test Ad 2 Body";
  info_2.ptr = 0.8;
  creative_ads.push_back(info_2);

  // Act
  SaveCreativeNotificationAds(creative_ads);

  // Assert
  CreativeNotificationAdList expected_creative_ads = creative_ads;

  const SegmentList segments = {"technology & computing-software"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       SaveCreativeNotificationAdsInBatches) {
  // Arrange
  database_table_->SetBatchSize(2);

  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.per_week = 4;
  info_1.per_month = 5;
  info_1.total_max = 6;
  info_1.value = 1.0;
  info_1.segment = "technology & computing-software";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = GURL("https://brave.com");
  info_1.title = "Test Ad 1 Title";
  info_1.body = "Test Ad 1 Body";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeNotificationAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 1;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 2;
  info_2.per_day = 3;
  info_2.per_week = 4;
  info_2.per_month = 5;
  info_2.total_max = 6;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = GURL("https://brave.com");
  info_2.title = "Test Ad 2 Title";
  info_2.body = "Test Ad 2 Body";
  info_2.ptr = 1.0;
  creative_ads.push_back(info_2);

  CreativeNotificationAdInfo info_3;
  info_3.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_3.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_3.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_3.start_at = DistantPast();
  info_3.end_at = DistantFuture();
  info_3.daily_cap = 1;
  info_3.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_3.priority = 2;
  info_3.per_day = 3;
  info_3.per_week = 4;
  info_3.per_month = 5;
  info_3.total_max = 6;
  info_3.value = 1.0;
  info_3.segment = "technology & computing-software";
  info_3.dayparts.push_back(daypart_info);
  info_3.geo_targets = {"US"};
  info_3.target_url = GURL("https://brave.com");
  info_3.title = "Test Ad 3 Title";
  info_3.body = "Test Ad 3 Body";
  info_3.ptr = 1.0;
  creative_ads.push_back(info_3);

  // Act
  SaveCreativeNotificationAds(creative_ads);

  // Assert
  CreativeNotificationAdList expected_creative_ads = creative_ads;

  const SegmentList segments = {"technology & computing-software"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       DoNotSaveDuplicateCreativeNotificationAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.per_week = 4;
  info.per_month = 5;
  info.total_max = 6;
  info.value = 1.0;
  info.segment = "technology & computing-software";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = GURL("https://brave.com");
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  SaveCreativeNotificationAds(creative_ads);

  // Act
  SaveCreativeNotificationAds(creative_ads);

  // Assert
  CreativeNotificationAdList expected_creative_ads = creative_ads;

  const SegmentList segments = {"technology & computing-software"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeDaypartInfo daypart_info_1;
  daypart_info_1.dow = "0";
  daypart_info_1.start_minute = 0;
  daypart_info_1.end_minute = 719;
  CreativeDaypartInfo daypart_info_2;
  daypart_info_2.dow = "1";
  daypart_info_2.start_minute = 720;
  daypart_info_2.end_minute = 1439;

  CreativeNotificationAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.per_week = 4;
  info_1.per_month = 5;
  info_1.total_max = 6;
  info_1.value = 1.0;
  info_1.segment = "technology & computing-software";
  info_1.dayparts.push_back(daypart_info_1);
  info_1.dayparts.push_back(daypart_info_2);
  info_1.geo_targets = {"US"};
  info_1.target_url = GURL("https://brave.com");
  info_1.title = "Test Ad 1 Title";
  info_1.body = "Test Ad 1 Body";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeNotificationAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 1;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 2;
  info_2.per_day = 3;
  info_2.per_week = 4;
  info_2.per_month = 5;
  info_2.total_max = 6;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info_1);
  info_2.dayparts.push_back(daypart_info_2);
  info_2.geo_targets = {"US-FL", "US-CA"};
  info_2.target_url = GURL("https://brave.com");
  info_2.title = "Test Ad 2 Title";
  info_2.body = "Test Ad 2 Body";
  info_2.ptr = 1.0;
  creative_ads.push_back(info_2);

  SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads = creative_ads;

  const SegmentList segments = {"technology & computing-software"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsForEmptySegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.per_week = 4;
  info.per_month = 5;
  info.total_max = 6;
  info.value = 1.0;
  info.segment = "technology & computing-software";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = GURL("https://brave.com");
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads;

  const SegmentList segments;

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsForNonExistentSegment) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.per_week = 4;
  info.per_month = 5;
  info.total_max = 6;
  info.value = 1.0;
  info.segment = "technology & computing-software";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = GURL("https://brave.com");
  info.title = "Test Ad 1 Title";
  info.body = "Test Ad 1 Body";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads;

  const SegmentList segments = {"food & drink"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsFromMultipleSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.per_week = 4;
  info_1.per_month = 5;
  info_1.total_max = 6;
  info_1.value = 1.0;
  info_1.segment = "technology & computing-software";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = GURL("https://brave.com");
  info_1.title = "Test Ad 1 Title";
  info_1.body = "Test Ad 1 Body";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeNotificationAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 1;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 2;
  info_2.per_day = 3;
  info_2.per_week = 4;
  info_2.per_month = 5;
  info_2.total_max = 6;
  info_2.value = 1.0;
  info_2.segment = "food & drink";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = GURL("https://brave.com");
  info_2.title = "Test Ad 2 Title";
  info_2.body = "Test Ad 2 Body";
  info_2.ptr = 1.0;
  creative_ads.push_back(info_2);

  CreativeNotificationAdInfo info_3;
  info_3.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_3.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_3.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_3.start_at = DistantPast();
  info_3.end_at = DistantFuture();
  info_3.daily_cap = 1;
  info_3.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_3.priority = 2;
  info_3.per_day = 3;
  info_3.per_week = 4;
  info_3.per_month = 5;
  info_3.total_max = 6;
  info_3.value = 1.0;
  info_3.segment = "automobiles";
  info_3.dayparts.push_back(daypart_info);
  info_3.geo_targets = {"US"};
  info_3.target_url = GURL("https://brave.com");
  info_3.title = "Test Ad 3 Title";
  info_3.body = "Test Ad 3 Body";
  info_3.ptr = 1.0;
  creative_ads.push_back(info_3);

  SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads;
  expected_creative_ads.push_back(info_1);
  expected_creative_ads.push_back(info_2);

  const std::vector<std::string> segments = {"technology & computing-software",
                                             "food & drink"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       GetNonExpiredCreativeNotificationAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = Now();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.per_week = 4;
  info_1.per_month = 5;
  info_1.total_max = 6;
  info_1.value = 1.0;
  info_1.segment = "technology & computing-software";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = GURL("https://brave.com");
  info_1.title = "Test Ad 1 Title";
  info_1.body = "Test Ad 1 Body";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeNotificationAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 1;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 2;
  info_2.per_day = 3;
  info_2.per_week = 4;
  info_2.per_month = 5;
  info_2.total_max = 6;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = GURL("https://brave.com");
  info_2.title = "Test Ad 2 Title";
  info_2.body = "Test Ad 2 Body";
  info_2.ptr = 1.0;
  creative_ads.push_back(info_2);

  SaveCreativeNotificationAds(creative_ads);

  // Act
  AdvanceClockBy(base::Hours(1));

  // Assert
  CreativeNotificationAdList expected_creative_ads;
  expected_creative_ads.push_back(info_2);

  const SegmentList segments = {"technology & computing-software"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest,
       GetCreativeNotificationAdsMatchingCaseInsensitiveSegments) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  const CreativeDaypartInfo daypart_info;
  CreativeNotificationAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.per_week = 4;
  info_1.per_month = 5;
  info_1.total_max = 6;
  info_1.value = 1.0;
  info_1.segment = "technology & computing-software";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = GURL("https://brave.com");
  info_1.title = "Test Ad 1 Title";
  info_1.body = "Test Ad 1 Body";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeNotificationAdInfo info_2;
  info_2.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_2.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_2.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 1;
  info_2.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_2.priority = 2;
  info_2.per_day = 3;
  info_2.per_week = 4;
  info_2.per_month = 5;
  info_2.total_max = 6;
  info_2.value = 1.0;
  info_2.segment = "food & drink";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = GURL("https://brave.com");
  info_2.title = "Test Ad 2 Title";
  info_2.body = "Test Ad 2 Body";
  info_2.ptr = 1.0;
  creative_ads.push_back(info_2);

  SaveCreativeNotificationAds(creative_ads);

  // Act

  // Assert
  CreativeNotificationAdList expected_creative_ads;
  expected_creative_ads.push_back(info_2);

  const SegmentList segments = {"FoOd & DrInK"};

  database_table_->GetForSegments(
      segments,
      base::BindOnce(
          [](const CreativeNotificationAdList& expected_creative_ads,
             const bool success, const SegmentList& /*segments*/,
             const CreativeNotificationAdList& creative_ads) {
            EXPECT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_creative_ads, creative_ads));
          },
          std::move(expected_creative_ads)));
}

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableTest, TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->GetTableName();

  // Assert
  const std::string expected_table_name = "creative_ad_notifications";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads::database::table
