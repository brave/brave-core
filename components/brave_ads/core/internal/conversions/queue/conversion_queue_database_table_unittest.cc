/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsConversionQueueDatabaseTableTest : public UnitTestBase {
 protected:
  ConversionQueue database_table_;
};

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveEmptyConversionQueue) {
  // Act
  test::SaveConversionQueue({});

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*conversion_queue_items=*/::testing::IsEmpty()));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveConversionQueue) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/1);

  // Act
  test::SaveConversionQueue(conversion_queue_items);

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       SaveDuplicateConversionQueueItems) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/1);
  test::SaveConversionQueue(conversion_queue_items);

  const ConversionQueueItemList expected_conversion_queue_items = {
      conversion_queue_items.front(), conversion_queue_items.front()};

  // Act
  test::SaveConversionQueue(conversion_queue_items);

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, expected_conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest, SaveConversionQueueInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      test::BuildConversionQueueItems(conversion, /*count=*/3);

  // Act
  test::SaveConversionQueue(conversion_queue_items);

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, conversion_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetConversionQueueItemForCreativeInstanceId) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueue(conversion_queue_items);

  // Act & Assert
  base::MockCallback<GetConversionQueueForCreativeInstanceIdCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ad_2.creative_instance_id,
                            ConversionQueueItemList{conversion_queue_item_2}));
  database_table_.GetForCreativeInstanceId(
      conversion_queue_item_2.conversion.creative_instance_id, callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetUnprocessedConversionQueueItems) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed,
                   /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_item_1.was_processed = true;
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueue(conversion_queue_items);

  // Act & Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ConversionQueueItemList{conversion_queue_item_2}));
  database_table_.GetUnprocessed(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       GetSortedConversionQueueSortedByTimeInAscendingOrder) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, DistantFuture()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/DistantFuture());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, DistantPast()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/DistantPast());
  conversion_queue_items.push_back(conversion_queue_item_2);

  const AdInfo ad_3 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_3 = BuildConversion(
      BuildAdEvent(ad_3, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_3);

  test::SaveConversionQueue(conversion_queue_items);

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

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueue(conversion_queue_items);

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

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueue(conversion_queue_items);

  const AdInfo ad_3 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_3 = BuildConversion(
      BuildAdEvent(ad_3, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
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

TEST_F(BraveAdsConversionQueueDatabaseTableTest, UpdateConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueue(conversion_queue_items);

  base::MockCallback<ResultCallback> update_callback;
  EXPECT_CALL(update_callback, Run(/*success=*/true));

  // Act
  database_table_.Update(conversion_queue_item_1, update_callback.Get());

  // Assert
  base::MockCallback<GetConversionQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ConversionQueueItemList{conversion_queue_item_2}));
  database_table_.GetUnprocessed(callback.Get());
}

TEST_F(BraveAdsConversionQueueDatabaseTableTest,
       DoNotUpdateMissingConversionQueueItem) {
  // Arrange
  ConversionQueueItemList conversion_queue_items;

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_1 = BuildConversion(
      BuildAdEvent(ad_1, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  ConversionQueueItemInfo conversion_queue_item_1 =
      BuildConversionQueueItem(conversion_1, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_2 = BuildConversion(
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_2 =
      BuildConversionQueueItem(conversion_2, /*process_at=*/Now());
  conversion_queue_items.push_back(conversion_queue_item_2);

  test::SaveConversionQueue(conversion_queue_items);

  const AdInfo ad_3 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const ConversionInfo conversion_3 = BuildConversion(
      BuildAdEvent(ad_3, ConfirmationType::kViewed, /*created_at=*/Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemInfo conversion_queue_item_3 =
      BuildConversionQueueItem(conversion_3, /*process_at=*/Now());

  base::MockCallback<ResultCallback> update_callback;
  EXPECT_CALL(update_callback, Run(/*success=*/true));

  // Act
  database_table_.Update(conversion_queue_item_3, update_callback.Get());

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
