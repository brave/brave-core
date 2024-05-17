/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"

#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsConfirmationQueueDatabaseTableTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("Mon, 8 Jul 1996 09:25"));
  }

  std::optional<ConfirmationInfo> BuildRewardConfirmation(
      const bool should_use_random_uuids) {
    const std::optional<ConfirmationInfo> confirmation =
        test::BuildRewardConfirmation(&token_generator_mock_,
                                      should_use_random_uuids);
    if (!confirmation) {
      return std::nullopt;
    }

    // The queue does not store dynamic user data for a confirmation due to the
    // token redemption process which rebuilds the confirmation. Hence, we must
    // regenerate the confirmation without the dynamic user data.
    return RebuildConfirmationWithoutDynamicUserData(*confirmation);
  }

  TokenGeneratorMock token_generator_mock_;

  ConfirmationQueue database_table_;
};

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, SaveEmptyConfirmationQueue) {
  // Act
  test::SaveConfirmationQueueItems({});

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*confirmation_queue_items=*/::testing::IsEmpty()));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, SaveConfirmationQueueItems) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(/*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const ConfirmationQueueItemList confirmation_queue_items =
      test::BuildConfirmationQueueItems(*confirmation, /*count=*/1);

  // Act
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, confirmation_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       SaveDuplicateConfirmationQueueItems) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(/*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemList confirmation_queue_items =
      test::BuildConfirmationQueueItems(*confirmation, /*count=*/1);
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  const ConfirmationQueueItemList expected_confirmation_queue_items = {
      confirmation_queue_items.front(), confirmation_queue_items.front()};

  // Act
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, expected_confirmation_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       SaveConfirmationQueueInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(/*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemList confirmation_queue_items =
      test::BuildConfirmationQueueItems(*confirmation, /*count=*/3);

  // Act
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, confirmation_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       GetNextConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/2);

  ConfirmationQueueItemList confirmation_queue_items;

  const std::optional<ConfirmationInfo> confirmation_1 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  const ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  const std::optional<ConfirmationInfo> confirmation_2 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Act & Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ConfirmationQueueItemList{
                                                  confirmation_queue_item_1}));
  database_table_.GetNext(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       GetSortedConfirmationQueueSortedByTimeInAscendingOrder) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/3);

  ConfirmationQueueItemList confirmation_queue_items;

  const std::optional<ConfirmationInfo> confirmation_1 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1,
                                 /*process_at=*/DistantFuture());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  const std::optional<ConfirmationInfo> confirmation_2 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/DistantPast());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  const std::optional<ConfirmationInfo> confirmation_3 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_3);
  const ConfirmationQueueItemInfo confirmation_queue_item_3 =
      BuildConfirmationQueueItem(*confirmation_3, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_3);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Act & Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ConfirmationQueueItemList{
                                                  confirmation_queue_item_2,
                                                  confirmation_queue_item_3,
                                                  confirmation_queue_item_1}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       DeleteConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/2);

  ConfirmationQueueItemList confirmation_queue_items;

  const std::optional<ConfirmationInfo> confirmation_1 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  const ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  const std::optional<ConfirmationInfo> confirmation_2 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  base::MockCallback<ResultCallback> delete_callback;
  EXPECT_CALL(delete_callback, Run(/*success=*/true));

  // Act
  database_table_.Delete(confirmation_queue_item_1.confirmation.transaction_id,
                         delete_callback.Get());

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ConfirmationQueueItemList{
                                                  confirmation_queue_item_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       DoNotDeleteMissingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/3);

  ConfirmationQueueItemList confirmation_queue_items;

  const std::optional<ConfirmationInfo> confirmation_1 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  const std::optional<ConfirmationInfo> confirmation_2 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  const std::optional<ConfirmationInfo> confirmation_3 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_3);
  const ConfirmationQueueItemInfo confirmation_queue_item_3 =
      BuildConfirmationQueueItem(*confirmation_3, /*process_at=*/Now());

  base::MockCallback<ResultCallback> delete_callback;
  EXPECT_CALL(delete_callback, Run(/*success=*/true));

  // Act
  database_table_.Delete(confirmation_queue_item_3.confirmation.transaction_id,
                         delete_callback.Get());

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, confirmation_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, RetryConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  ConfirmationQueueItemList confirmation_queue_items;

  const std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation);
  ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  base::MockCallback<ResultCallback> retry_callback;
  EXPECT_CALL(retry_callback, Run(/*success=*/true));

  const ScopedRandTimeDeltaSetterForTesting scoped_rand_time_delta(
      base::Minutes(7));

  // Act
  database_table_.Retry(confirmation_queue_item.confirmation.transaction_id,
                        retry_callback.Get());

  // Assert
  confirmation_queue_item.process_at = Now() + base::Minutes(7);
  confirmation_queue_item.retry_count = 1;
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ConfirmationQueueItemList{
                                                  confirmation_queue_item}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       DoNotRetryMissingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/3);

  ConfirmationQueueItemList confirmation_queue_items;

  const std::optional<ConfirmationInfo> confirmation_1 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  const std::optional<ConfirmationInfo> confirmation_2 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  const std::optional<ConfirmationInfo> confirmation_3 =
      BuildRewardConfirmation(/*should_use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_3);
  const ConfirmationQueueItemInfo confirmation_queue_item_3 =
      BuildConfirmationQueueItem(*confirmation_3, /*process_at=*/Now());

  base::MockCallback<ResultCallback> retry_callback;
  EXPECT_CALL(retry_callback, Run(/*success=*/true));

  // Act
  database_table_.Retry(confirmation_queue_item_3.confirmation.transaction_id,
                        retry_callback.Get());

  // Assert
  base::MockCallback<GetConfirmationQueueCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, confirmation_queue_items));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("confirmation_queue", database_table_.GetTableName());
}

}  // namespace brave_ads::database::table
