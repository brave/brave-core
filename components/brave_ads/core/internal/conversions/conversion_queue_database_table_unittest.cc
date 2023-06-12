/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_database_table.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsConversionQueueDatabaseTableTest : public UnitTestBase {
 protected:
  ConversionQueue database_table_;
};

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveEmptyConversionQueue) {
  // Arrange
  const ConversionQueueItemList conversion_queue_items;

  // Act
  SaveConversionQueueItems(conversion_queue_items);

  // Assert
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveConversionQueue) {
  // Arrange

  // Act
  const ConversionQueueItemList conversion_queue_items =
      BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                       kConversionAdvertiserPublicKey,
                                       /*should_use_random_uuids*/ false,
                                       /*count*/ 2);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       SaveDuplicateConversionQueueItems) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item);

  SaveConversionQueueItems(conversion_queue_items);

  // Act
  SaveConversionQueueItems(conversion_queue_items);

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item, conversion_queue_item};

  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      expected_conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       SaveConversionQueueItemsInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  // Act
  const ConversionQueueItemList conversion_queue_items =
      BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                       kConversionAdvertiserPublicKey,
                                       /*should_use_random_uuids*/ true,
                                       /*count*/ 3);

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetConversionQueueItemForCreativeInstanceId) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItems(conversion_queue_items);

  // Act

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item_2};

  database_table_.GetForCreativeInstanceId(
      conversion_queue_item_2.creative_instance_id,
      base::BindOnce(
          [](const ConversionQueueItemList& expected_conversion_queue_items,
             const bool success, const std::string& /*creative_instance_id*/,
             const ConversionQueueItemList& conversion_queue_items) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
          },
          expected_conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetUnprocessedConversionQueueItems) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo conversion_queue_item_1 = BuildConversionQueueItem(
      AdType::kNotificationAd, kConversionId, kConversionAdvertiserPublicKey,
      /*should_use_random_uuids*/ true);
  conversion_queue_item_1.process_at = DistantPast();
  conversion_queue_item_1.was_processed = true;
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItems(conversion_queue_items);

  // Act

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item_2};

  database_table_.GetUnprocessed(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      expected_conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetSortedConversionQueueSortedByTimeInAscendingOrder) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo conversion_queue_item_1 = BuildConversionQueueItem(
      AdType::kNotificationAd, kConversionId, kConversionAdvertiserPublicKey,
      /*should_use_random_uuids*/ true);
  conversion_queue_item_1.process_at = DistantFuture();
  conversion_queue_items.push_back(conversion_queue_item_1);

  ConversionQueueItemInfo conversion_queue_item_2 = BuildConversionQueueItem(
      AdType::kNotificationAd, kConversionId, kConversionAdvertiserPublicKey,
      /*should_use_random_uuids*/ true);
  conversion_queue_item_2.process_at = DistantPast();
  conversion_queue_items.push_back(conversion_queue_item_2);

  ConversionQueueItemInfo conversion_queue_item_3 = BuildConversionQueueItem(
      AdType::kNotificationAd, kConversionId, kConversionAdvertiserPublicKey,
      /*should_use_random_uuids*/ true);
  conversion_queue_item_3.process_at = Now();
  conversion_queue_items.push_back(conversion_queue_item_3);

  SaveConversionQueueItems(conversion_queue_items);

  // Act

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item_2, conversion_queue_item_3,
      conversion_queue_item_1};

  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      expected_conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, DeleteConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo conversion_queue_item_1 = BuildConversionQueueItem(
      AdType::kNotificationAd, kConversionId, kConversionAdvertiserPublicKey,
      /*should_use_random_uuids*/ true);
  conversion_queue_item_1.process_at = DistantPast();
  conversion_queue_items.push_back(conversion_queue_item_1);

  ConversionQueueItemInfo conversion_queue_item_2 = BuildConversionQueueItem(
      AdType::kNotificationAd, kConversionId, kConversionAdvertiserPublicKey,
      /*should_use_random_uuids*/ true);
  conversion_queue_item_2.process_at = Now();
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItems(conversion_queue_items);

  // Act
  database_table_.Delete(
      conversion_queue_item_1,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item_2};

  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      expected_conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       DeleteInvalidConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItems(conversion_queue_items);

  // Act
  const ConversionQueueItemInfo invalid_conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);

  database_table_.Delete(
      invalid_conversion_queue_item,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, UpdateConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItems(conversion_queue_items);

  // Act
  database_table_.Update(
      conversion_queue_item_1,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item_2};

  database_table_.GetUnprocessed(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      expected_conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       UpdateInvalidConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItems(conversion_queue_items);

  // Act
  const ConversionQueueItemInfo invalid_conversion_queue_item =
      BuildConversionQueueItem(AdType::kNotificationAd, kConversionId,
                               kConversionAdvertiserPublicKey,
                               /*should_use_random_uuids*/ true);

  database_table_.Update(
      invalid_conversion_queue_item,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  database_table_.GetAll(base::BindOnce(
      [](const ConversionQueueItemList& expected_conversion_queue_items,
         const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      },
      conversion_queue_items));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, TableName) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("conversion_queue", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
