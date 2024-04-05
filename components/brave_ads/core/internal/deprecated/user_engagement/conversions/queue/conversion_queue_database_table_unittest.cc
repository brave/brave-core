/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/conversion_queue_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/conversion/conversion_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsConversionQueueDatabaseTableTest : public UnitTestBase {
 protected:
  ConversionQueue database_table_;
};

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveEmptyConversionQueue) {
  // Act
  test::SaveConversionQueueItems({});

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*conversion_queue_items=*/::testing::IsEmpty()));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveConversionQueueItems) {
  // Arrange
  const ConversionInfo conversion = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/false);
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/1);

  // Act
  test::SaveConversionQueueItems(conversion_queue_items);

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       SaveDuplicateConversionQueueItems) {
  // Arrange
  const ConversionInfo conversion = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/false);
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/1);
  test::SaveConversionQueueItems(conversion_queue_items);

  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_items.front(), conversion_queue_items.front()};

  // Act
  test::SaveConversionQueueItems(conversion_queue_items);

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, expected_conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveConversionQueueInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const ConversionInfo conversion = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/false);
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/3);

  // Act
  test::SaveConversionQueueItems(conversion_queue_items);

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetConversionQueueItemForCreativeInstanceId) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueueItems(conversion_queue_items);

  // Act & Assert
  base::MockCallback<GetConversionQueueForCreativeInstanceIdCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_2.creative_instance_id,
                            ConversionQueueItemList{conversion_queue_item_2}));
  database_table_.GetForCreativeInstanceId(
      conversion_queue_item_2.conversion.creative_instance_id, callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, GetNextConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_item_1.was_processed = true;
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueueItems(conversion_queue_items);

  // Act & Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ConversionQueueItemList{conversion_queue_item_2}));
  database_table_.GetNext(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetSortedConversionQueueSortedByTimeInAscendingOrder) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/DistantFuture());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/DistantPast());
  conversion_queue_items.push_back(conversion_queue_item_2);

  const ConversionInfo conversion_3 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_3);

  test::SaveConversionQueueItems(conversion_queue_items);

  // Act & Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ConversionQueueItemList{conversion_queue_item_2,
                                                    conversion_queue_item_3,
                                                    conversion_queue_item_1}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, DeleteConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueueItems(conversion_queue_items);

  base::MockCallback<ResultCallback> delete_callback;
  EXPECT_CALL(delete_callback, Run(/*success=*/true));

  // Act
  database_table_.Delete(conversion_queue_item_1, delete_callback.Get());

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ConversionQueueItemList{conversion_queue_item_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       DoNotDeleteMissingConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueueItems(conversion_queue_items);

  const ConversionInfo conversion_3 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at=*/Now());

  base::MockCallback<ResultCallback> delete_callback;
  EXPECT_CALL(delete_callback, Run(/*success=*/true));

  // Act
  database_table_.Delete(conversion_queue_item_3, delete_callback.Get());

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       MarkConversionQueueItemAsProcessed) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  // Move the clock forward to ensure that the next conversion occurs after the
  // first one.
  AdvanceClockBy(base::Milliseconds(1));

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueueItems(conversion_queue_items);

  base::MockCallback<ResultCallback> mark_as_processed_callback;
  EXPECT_CALL(mark_as_processed_callback, Run(/*success=*/true));

  // Act
  conversion_queue_item_1.was_processed = true;
  database_table_.MarkAsProcessed(conversion_queue_item_1,
                                  mark_as_processed_callback.Get());

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ConversionQueueItemList{conversion_queue_item_1,
                                                    conversion_queue_item_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       DoNotMarkMissingConversionQueueItemAsProcessed) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const ConversionInfo conversion_1 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const ConversionInfo conversion_2 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueueItems(conversion_queue_items);

  const ConversionInfo conversion_3 = test::BuildVerifiableConversion(
      AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey},
      /*should_use_random_uuids=*/true);
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at=*/Now());

  base::MockCallback<ResultCallback> mark_as_processed_callback;
  EXPECT_CALL(mark_as_processed_callback, Run(/*success=*/true));

  // Act
  database_table_.MarkAsProcessed(conversion_queue_item_3,
                                  mark_as_processed_callback.Get());

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("conversion_queue", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
