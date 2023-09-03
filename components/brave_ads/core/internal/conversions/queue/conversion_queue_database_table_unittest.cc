/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_database_table.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"

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
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Assert
  database_table_.GetAll(
      base::BindOnce([](const bool success,
                        const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(conversion_queue_items.empty());
      }));
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveConversionQueue) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);

  // Act
  SaveConversionQueueItemsForTesting(conversion_queue_items);

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
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Act
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_items.front(), conversion_queue_items.front()};

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

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 3);

  // Act
  SaveConversionQueueItemsForTesting(conversion_queue_items);

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

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Act

  // Assert
  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_item_2};

  database_table_.GetForCreativeInstanceId(
      conversion_queue_item_2.conversion.creative_instance_id,
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

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed,
                   /*process_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ Now());
  conversion_queue_item_1.was_processed = true;
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

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

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, DistantFuture()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ DistantFuture());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, DistantPast()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ DistantPast());
  conversion_queue_items.push_back(conversion_queue_item_2);

  const AdInfo ad_3 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_3 = BuildConversion(
      BuildAdEvent(ad_3, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_3);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

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

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

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
       DoNotDeleteMissingConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const AdInfo ad_3 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_3 = BuildConversion(
      BuildAdEvent(ad_3, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at*/ Now());

  // Act
  database_table_.Delete(
      conversion_queue_item_3,
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

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

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
       DoNotUpdateMissingConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at*/ Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const AdInfo ad_3 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  const ConversionInfo conversion_3 = BuildConversion(
      BuildAdEvent(ad_3, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at*/ Now());

  // Act
  database_table_.Update(
      conversion_queue_item_3,
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
