/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"

#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeInlineContentAdsDatabaseTableTest : public UnitTestBase {
 protected:
  BatAdsCreativeInlineContentAdsDatabaseTableTest()
      : database_table_(
            std::make_unique<database::table::CreativeInlineContentAds>()) {}

  ~BatAdsCreativeInlineContentAdsDatabaseTableTest() override = default;

  void Save(const CreativeInlineContentAdList& creative_ads) {
    database_table_->Save(creative_ads,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::CreativeInlineContentAds> database_table_;
};

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       SaveEmptyCreativeInlineContentAds) {
  // Arrange
  const CreativeInlineContentAdList creative_ads = {};

  // Act
  Save(creative_ads);

  // Assert
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       SaveCreativeInlineContentAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.total_max = 4;
  info_1.value = 1.0;
  info_1.segment = "food & drink";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = "https://brave.com/1";
  info_1.title = "Test Ad 1 Title";
  info_1.description = "Test Ad 1 Description";
  info_1.image_url = "https://www.brave.com/1/image.png";
  info_1.dimensions = "200x100";
  info_1.cta_text = "Call to Action Text 1";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeInlineContentAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 5;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 6;
  info_2.per_day = 7;
  info_2.total_max = 8;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = "https://brave.com/2";
  info_2.title = "Test Ad 2 Title";
  info_2.description = "Test Ad 2 Description";
  info_2.image_url = "https://www.brave.com/2/image.png";
  info_2.dimensions = "200x100";
  info_2.cta_text = "Call to Action Text 2";
  info_2.ptr = 0.9;
  creative_ads.push_back(info_2);

  // Act
  Save(creative_ads);

  // Assert
  const CreativeInlineContentAdList expected_creative_ads = creative_ads;

  database_table_->GetAll([&expected_creative_ads](
                              const bool success, const SegmentList& segments,
                              const CreativeInlineContentAdList& creative_ads) {
    EXPECT_TRUE(success);
    EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
  });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       SaveCreativeInlineContentAdsInBatches) {
  // Arrange
  database_table_->set_batch_size(2);

  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.total_max = 4;
  info_1.value = 1.0;
  info_1.segment = "food & drink";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = "https://brave.com/1";
  info_1.title = "Test Ad 1 Title";
  info_1.description = "Test Ad 1 Description";
  info_1.image_url = "https://www.brave.com/1/image.png";
  info_1.dimensions = "200x100";
  info_1.cta_text = "Call to Action Text 1";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeInlineContentAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 5;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 6;
  info_2.per_day = 7;
  info_2.total_max = 8;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = "https://brave.com/2";
  info_2.title = "Test Ad 2 Title";
  info_2.description = "Test Ad 2 Description";
  info_2.image_url = "https://www.brave.com/2/image.png";
  info_2.dimensions = "200x100";
  info_2.cta_text = "Call to Action Text 2";
  info_2.ptr = 0.9;
  creative_ads.push_back(info_2);

  CreativeInlineContentAdInfo info_3;
  info_3.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_3.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_3.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_3.start_at = DistantPast();
  info_3.end_at = DistantFuture();
  info_3.daily_cap = 1;
  info_3.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_3.priority = 2;
  info_3.per_day = 3;
  info_3.total_max = 4;
  info_3.value = 1.0;
  info_3.segment = "finance-banking";
  info_3.dayparts.push_back(daypart_info);
  info_3.geo_targets = {"US"};
  info_3.target_url = "https://brave.com/3";
  info_3.title = "Test Ad 3 Title";
  info_3.description = "Test Ad 3 Description";
  info_3.image_url = "https://www.brave.com/3/image.png";
  info_3.dimensions = "200x100";
  info_3.cta_text = "Call to Action Text 3";
  info_3.ptr = 1.0;
  creative_ads.push_back(info_3);

  // Act
  Save(creative_ads);

  // Assert
  const CreativeInlineContentAdList expected_creative_ads = creative_ads;

  database_table_->GetAll([&expected_creative_ads](
                              const bool success, const SegmentList& segments,
                              const CreativeInlineContentAdList& creative_ads) {
    EXPECT_TRUE(success);
    EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
  });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       DoNotSaveDuplicateCreativeInlineContentAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.total_max = 4;
  info.value = 1.0;
  info.segment = "food & drink";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad Title";
  info.description = "Test Ad Description";
  info.image_url = "https://www.brave.com/image.png";
  info.dimensions = "200x100";
  info.cta_text = "Call to Action Text";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act
  Save(creative_ads);

  // Assert
  const CreativeInlineContentAdList expected_creative_ads = creative_ads;

  database_table_->GetAll([&expected_creative_ads](
                              const bool success, const SegmentList& segments,
                              const CreativeInlineContentAdList& creative_ads) {
    EXPECT_TRUE(success);
    EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
  });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetForSegmentsAndDimensions) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.total_max = 4;
  info_1.value = 1.0;
  info_1.segment = "food & drink";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = "https://brave.com/1";
  info_1.title = "Test Ad 1 Title";
  info_1.description = "Test Ad 1 Description";
  info_1.image_url = "https://www.brave.com/1/image.png";
  info_1.dimensions = "200x100";
  info_1.cta_text = "Call to Action Text 1";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeInlineContentAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 5;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 6;
  info_2.per_day = 7;
  info_2.total_max = 8;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = "https://brave.com/2";
  info_2.title = "Test Ad 2 Title";
  info_2.description = "Test Ad 2 Description";
  info_2.image_url = "https://www.brave.com/2/image.png";
  info_2.dimensions = "200x100";
  info_2.cta_text = "Call to Action Text 2";
  info_2.ptr = 0.9;
  creative_ads.push_back(info_2);

  Save(creative_ads);

  // Act

  // Assert
  const CreativeInlineContentAdList expected_creative_ads = creative_ads;

  database_table_->GetAll([&expected_creative_ads](
                              const bool success, const SegmentList& segments,
                              const CreativeInlineContentAdList& creative_ads) {
    EXPECT_TRUE(success);
    EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
  });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForCreativeInstanceId) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info_1;
  daypart_info_1.dow = "0";
  daypart_info_1.start_minute = 0;
  daypart_info_1.end_minute = 719;
  CreativeDaypartInfo daypart_info_2;
  daypart_info_2.dow = "1";
  daypart_info_2.start_minute = 720;
  daypart_info_2.end_minute = 1439;

  CreativeInlineContentAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.total_max = 4;
  info.value = 1.0;
  info.segment = "food & drink";
  info.dayparts.push_back(daypart_info_1);
  info.dayparts.push_back(daypart_info_2);
  info.geo_targets = {"US-FL", "US-CA"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad Title";
  info.description = "Test Ad Description";
  info.image_url = "https://www.brave.com/image.png";
  info.dimensions = "200x100";
  info.cta_text = "Call to Action Text";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act

  // Assert
  const CreativeInlineContentAdInfo expected_creative_ad = info;

  const std::string creative_instance_id =
      "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  database_table_->GetForCreativeInstanceId(
      creative_instance_id,
      [&expected_creative_ad](const bool success,
                              const std::string& creative_instance_id,
                              const CreativeInlineContentAdInfo& creative_ad) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_creative_ad, creative_ad);
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForNonExistentCreativeInstanceId) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.total_max = 4;
  info.value = 1.0;
  info.segment = "food & drink";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad Title";
  info.description = "Test Ad Description";
  info.image_url = "https://www.brave.com/image.png";
  info.dimensions = "200x100";
  info.cta_text = "Call to Action Text";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act

  // Assert
  const std::string creative_instance_id =
      "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";

  database_table_->GetForCreativeInstanceId(
      creative_instance_id,
      [](const bool success, const std::string& creative_instance_id,
         const CreativeInlineContentAdInfo& creative_ad) {
        EXPECT_FALSE(success);
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForEmptySegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.total_max = 4;
  info.value = 1.0;
  info.segment = "food & drink";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad Title";
  info.description = "Test Ad Description";
  info.image_url = "https://www.brave.com/image.png";
  info.dimensions = "200x100";
  info.cta_text = "Call to Action Text";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act

  // Assert
  const CreativeInlineContentAdList expected_creative_ads = {};

  const SegmentList segments = {};

  database_table_->GetForSegmentsAndDimensions(
      segments, "200x100",
      [&expected_creative_ads](
          const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsForNonExistentCategory) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.start_at = DistantPast();
  info.end_at = DistantFuture();
  info.daily_cap = 1;
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.priority = 2;
  info.per_day = 3;
  info.total_max = 4;
  info.value = 1.0;
  info.segment = "food & drink";
  info.dayparts.push_back(daypart_info);
  info.geo_targets = {"US"};
  info.target_url = "https://brave.com";
  info.title = "Test Ad Title";
  info.description = "Test Ad Description";
  info.image_url = "https://www.brave.com/image.png";
  info.dimensions = "200x100";
  info.cta_text = "Call to Action Text";
  info.ptr = 1.0;
  creative_ads.push_back(info);

  Save(creative_ads);

  // Act

  // Assert
  const CreativeInlineContentAdList expected_creative_ads = {};

  const SegmentList segments = {"technology & computing"};

  database_table_->GetForSegmentsAndDimensions(
      segments, "200x100",
      [&expected_creative_ads](
          const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsFromMultipleSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.total_max = 4;
  info_1.value = 1.0;
  info_1.segment = "food & drink";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = "https://brave.com/1";
  info_1.title = "Test Ad 1 Title";
  info_1.description = "Test Ad 1 Description";
  info_1.image_url = "https://www.brave.com/1/image.png";
  info_1.dimensions = "200x100";
  info_1.cta_text = "Call to Action Text 1";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeInlineContentAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 5;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 6;
  info_2.per_day = 7;
  info_2.total_max = 8;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = "https://brave.com/2";
  info_2.title = "Test Ad 2 Title";
  info_2.description = "Test Ad 2 Description";
  info_2.image_url = "https://www.brave.com/2/image.png";
  info_2.dimensions = "200x100";
  info_2.cta_text = "Call to Action Text 2";
  info_2.ptr = 0.9;
  creative_ads.push_back(info_2);

  CreativeInlineContentAdInfo info_3;
  info_3.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_3.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_3.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_3.start_at = DistantPast();
  info_3.end_at = DistantFuture();
  info_3.daily_cap = 1;
  info_3.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_3.priority = 2;
  info_3.per_day = 3;
  info_3.total_max = 4;
  info_3.value = 1.0;
  info_3.segment = "finance-banking";
  info_3.dayparts.push_back(daypart_info);
  info_3.geo_targets = {"US"};
  info_3.target_url = "https://brave.com/3";
  info_3.title = "Test Ad 3 Title";
  info_3.description = "Test Ad 3 Description";
  info_3.image_url = "https://www.brave.com/3/image.png";
  info_3.dimensions = "200x100";
  info_3.cta_text = "Call to Action Text 3";
  info_3.ptr = 1.0;
  creative_ads.push_back(info_3);

  Save(creative_ads);

  // Act

  // Assert
  CreativeInlineContentAdList expected_creative_ads;
  expected_creative_ads.push_back(info_1);
  expected_creative_ads.push_back(info_2);

  const SegmentList segments = {"technology & computing-software",
                                "food & drink"};

  database_table_->GetForSegmentsAndDimensions(
      segments, "200x100",
      [&expected_creative_ads](
          const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetNonExpiredCreativeInlineContentAds) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = Now();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.total_max = 4;
  info_1.value = 1.0;
  info_1.segment = "food & drink";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = "https://brave.com/1";
  info_1.title = "Test Ad 1 Title";
  info_1.description = "Test Ad 1 Description";
  info_1.image_url = "https://www.brave.com/1/image.png";
  info_1.dimensions = "200x100";
  info_1.cta_text = "Call to Action Text 1";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeInlineContentAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 5;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 6;
  info_2.per_day = 7;
  info_2.total_max = 8;
  info_2.value = 1.0;
  info_2.segment = "food & drink";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = "https://brave.com/2";
  info_2.title = "Test Ad 2 Title";
  info_2.description = "Test Ad 2 Description";
  info_2.image_url = "https://www.brave.com/2/image.png";
  info_2.dimensions = "200x100";
  info_2.cta_text = "Call to Action Text 2";
  info_2.ptr = 0.9;
  creative_ads.push_back(info_2);

  Save(creative_ads);

  // Act
  FastForwardClockBy(base::Hours(1));

  // Assert
  CreativeInlineContentAdList expected_creative_ads;
  expected_creative_ads.push_back(info_2);

  const SegmentList segments = {"food & drink"};

  database_table_->GetForSegmentsAndDimensions(
      segments, "200x100",
      [&expected_creative_ads](
          const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest,
       GetCreativeInlineContentAdsMatchingCaseInsensitiveSegments) {
  // Arrange
  CreativeInlineContentAdList creative_ads;

  CreativeDaypartInfo daypart_info;
  CreativeInlineContentAdInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.start_at = DistantPast();
  info_1.end_at = DistantFuture();
  info_1.daily_cap = 1;
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.priority = 2;
  info_1.per_day = 3;
  info_1.total_max = 4;
  info_1.value = 1.0;
  info_1.segment = "food & drink";
  info_1.dayparts.push_back(daypart_info);
  info_1.geo_targets = {"US"};
  info_1.target_url = "https://brave.com/1";
  info_1.title = "Test Ad 1 Title";
  info_1.description = "Test Ad 1 Description";
  info_1.image_url = "https://www.brave.com/1/image.png";
  info_1.dimensions = "200x100";
  info_1.cta_text = "Call to Action Text 1";
  info_1.ptr = 1.0;
  creative_ads.push_back(info_1);

  CreativeInlineContentAdInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.start_at = DistantPast();
  info_2.end_at = DistantFuture();
  info_2.daily_cap = 5;
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.priority = 6;
  info_2.per_day = 7;
  info_2.total_max = 8;
  info_2.value = 1.0;
  info_2.segment = "technology & computing-software";
  info_2.dayparts.push_back(daypart_info);
  info_2.geo_targets = {"US"};
  info_2.target_url = "https://brave.com/2";
  info_2.title = "Test Ad 2 Title";
  info_2.description = "Test Ad 2 Description";
  info_2.image_url = "https://www.brave.com/2/image.png";
  info_2.dimensions = "200x100";
  info_2.cta_text = "Call to Action Text 2";
  info_2.ptr = 0.9;
  creative_ads.push_back(info_2);

  Save(creative_ads);

  // Act

  // Assert
  CreativeInlineContentAdList expected_creative_ads;
  expected_creative_ads.push_back(info_1);

  const SegmentList segments = {"FoOd & DrInK"};

  database_table_->GetForSegmentsAndDimensions(
      segments, "200x100",
      [&expected_creative_ads](
          const bool success, const SegmentList& segments,
          const CreativeInlineContentAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_creative_ads, creative_ads));
      });
}

TEST_F(BatAdsCreativeInlineContentAdsDatabaseTableTest, TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->GetTableName();

  // Assert
  const std::string expected_table_name = "creative_inline_content_ads";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
