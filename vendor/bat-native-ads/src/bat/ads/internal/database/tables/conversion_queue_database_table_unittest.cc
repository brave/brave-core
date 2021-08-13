/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"

#include <memory>

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConversionQueueDatabaseTableTest : public UnitTestBase {
 protected:
  BatAdsConversionQueueDatabaseTableTest()
      : database_table_(std::make_unique<database::table::ConversionQueue>()) {}

  ~BatAdsConversionQueueDatabaseTableTest() override = default;

  void Save(const ConversionQueueItemList& conversion_queue_items) {
    database_table_->Save(conversion_queue_items,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::ConversionQueue> database_table_;
};

TEST_F(BatAdsConversionQueueDatabaseTableTest, SaveEmptyConversionQueue) {
  // Arrange
  const ConversionQueueItemList conversion_queue_items = {};

  // Act
  Save(conversion_queue_items);

  // Assert
}

TEST_F(BatAdsConversionQueueDatabaseTableTest, SaveConversionQueue) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.timestamp = DistantPast();
  conversion_queue_items.push_back(info_1);

  ConversionQueueItemInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.timestamp = Now();
  conversion_queue_items.push_back(info_2);

  // Act
  Save(conversion_queue_items);

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items =
      conversion_queue_items;

  database_table_->GetAll(
      [&expected_conversion_queue_items](
          const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest,
       SaveDuplicateConversionQueueItems) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.timestamp = Now();
  conversion_queue_items.push_back(info);

  Save(conversion_queue_items);

  // Act
  Save(conversion_queue_items);

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {info, info};

  database_table_->GetAll(
      [&expected_conversion_queue_items](
          const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest,
       SaveConversionQueueItemsInBatches) {
  // Arrange
  database_table_->set_batch_size(2);

  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.timestamp = DistantPast();
  conversion_queue_items.push_back(info_1);

  ConversionQueueItemInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.timestamp = Now();
  conversion_queue_items.push_back(info_2);

  ConversionQueueItemInfo info_3;
  info_3.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_3.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_3.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_3.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_3.timestamp = DistantFuture();
  conversion_queue_items.push_back(info_3);

  // Act
  Save(conversion_queue_items);

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items =
      conversion_queue_items;

  database_table_->GetAll(
      [&expected_conversion_queue_items](
          const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest,
       GetConversionQueueItemForCreativeInstanceId) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.timestamp = DistantPast();
  conversion_queue_items.push_back(info_1);

  ConversionQueueItemInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.timestamp = Now();
  conversion_queue_items.push_back(info_2);

  Save(conversion_queue_items);

  // Act

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {info_2};

  const std::string creative_instance_id =
      "eaa6224a-876d-4ef8-a384-9ac34f238631";

  database_table_->GetForCreativeInstanceId(
      creative_instance_id,
      [&expected_conversion_queue_items](
          const bool success, const std::string& creative_instance_id,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest,
       GetSortedConversionQueueSortedByTimestampInAscendingOrder) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info_1;
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.timestamp = DistantFuture();
  conversion_queue_items.push_back(info_1);

  ConversionQueueItemInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.timestamp = DistantPast();
  conversion_queue_items.push_back(info_2);

  ConversionQueueItemInfo info_3;
  info_3.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  info_3.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  info_3.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  info_3.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  info_3.timestamp = Now();
  conversion_queue_items.push_back(info_3);

  Save(conversion_queue_items);

  // Act

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      info_2, info_3, info_1};

  database_table_->GetAll(
      [&expected_conversion_queue_items](
          const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest, DeleteConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.timestamp = DistantPast();
  conversion_queue_items.push_back(info_1);

  ConversionQueueItemInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.timestamp = Now();
  conversion_queue_items.push_back(info_2);

  Save(conversion_queue_items);

  // Act
  database_table_->Delete(info_1,
                          [](const bool success) { ASSERT_TRUE(success); });

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {info_2};

  database_table_->GetAll(
      [&expected_conversion_queue_items](
          const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest,
       DeleteInvalidConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info_1;
  info_1.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info_1.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info_1.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info_1.timestamp = DistantPast();
  conversion_queue_items.push_back(info_1);

  ConversionQueueItemInfo info_2;
  info_2.creative_instance_id = "eaa6224a-876d-4ef8-a384-9ac34f238631";
  info_2.creative_set_id = "184d1fdd-8e18-4baa-909c-9a3cb62cc7b1";
  info_2.campaign_id = "d1d4a649-502d-4e06-b4b8-dae11c382d26";
  info_2.advertiser_id = "8e3fac86-ce50-4409-ae29-9aa5636aa9a2";
  info_2.timestamp = Now();
  conversion_queue_items.push_back(info_2);

  Save(conversion_queue_items);

  // Act
  ConversionQueueItemInfo invalid_conversion_queue_item;
  invalid_conversion_queue_item.creative_instance_id =
      "a1ac44c2-675f-43e6-ab6d-500614cafe63";
  invalid_conversion_queue_item.creative_set_id =
      "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
  invalid_conversion_queue_item.campaign_id =
      "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
  invalid_conversion_queue_item.advertiser_id =
      "9a11b60f-e29d-4446-8d1f-318311e36e0a";
  invalid_conversion_queue_item.timestamp = Now();

  database_table_->Delete(invalid_conversion_queue_item,
                          [](const bool success) { ASSERT_TRUE(success); });

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items =
      conversion_queue_items;

  database_table_->GetAll(
      [&expected_conversion_queue_items](
          const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        EXPECT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });
}

TEST_F(BatAdsConversionQueueDatabaseTableTest, TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->get_table_name();

  // Assert
  const std::string expected_table_name = "conversion_queue";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
