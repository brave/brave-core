/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/test/confirmation_queue_item_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/test/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/test/confirmation_user_data_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/test/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/test/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsConfirmationQueueDatabaseTableTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::MockConfirmationUserData();

    AdvanceClockTo(test::TimeFromUTCString("Mon, 8 Jul 1996 09:25"));
  }

  ConfirmationQueue database_table_;
};

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, SaveEmptyConfirmationQueue) {
  // Act
  test::SaveConfirmationQueueItems({});

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, confirmation_queue_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(confirmation_queue_items, ::testing::IsEmpty());
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, SaveConfirmationQueueItems) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const ConfirmationQueueItemList confirmation_queue_items =
      test::BuildConfirmationQueueItems(*confirmation, /*count=*/1);

  // Act
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(confirmation_queue_items, got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       SaveDuplicateConfirmationQueueItems) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemList confirmation_queue_items =
      test::BuildConfirmationQueueItems(*confirmation, /*count=*/1);
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Act
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Assert
  const ConfirmationQueueItemList expected_confirmation_queue_items = {
      confirmation_queue_items.front(), confirmation_queue_items.front()};
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(expected_confirmation_queue_items, got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       SaveConfirmationQueueInBatches) {
  // Arrange
  database_table_.SetBatchSize(2);

  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemList confirmation_queue_items =
      test::BuildConfirmationQueueItems(*confirmation, /*count=*/3);

  // Act
  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(confirmation_queue_items, got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       GetNextConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/2);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  const ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Act & Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetNext(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((ConfirmationQueueItemList{confirmation_queue_item_1}), got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       GetSortedConfirmationQueueSortedByTimeInAscendingOrder) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/3);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1,
                                 /*process_at=*/test::DistantFuture());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2,
                                 /*process_at=*/test::DistantPast());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  std::optional<ConfirmationInfo> confirmation_3 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_3);
  const ConfirmationQueueItemInfo confirmation_queue_item_3 =
      BuildConfirmationQueueItem(*confirmation_3, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_3);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Act & Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((ConfirmationQueueItemList{confirmation_queue_item_2,
                                       confirmation_queue_item_3,
                                       confirmation_queue_item_1}),
            got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       DeleteConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/2);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  const ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  // Act
  base::test::TestFuture<bool> test_future_delete;
  database_table_.Delete(confirmation_queue_item_1.confirmation.transaction_id,
                         test_future_delete.GetCallback());
  EXPECT_TRUE(test_future_delete.Get());

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((ConfirmationQueueItemList{confirmation_queue_item_2}), got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       DoNotDeleteMissingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/3);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  std::optional<ConfirmationInfo> confirmation_3 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_3);
  const ConfirmationQueueItemInfo confirmation_queue_item_3 =
      BuildConfirmationQueueItem(*confirmation_3, /*process_at=*/test::Now());

  // Act
  base::test::TestFuture<bool> test_future_delete;
  database_table_.Delete(confirmation_queue_item_3.confirmation.transaction_id,
                         test_future_delete.GetCallback());
  EXPECT_TRUE(test_future_delete.Get());

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(confirmation_queue_items, got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest, RetryConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation);
  ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  const ScopedRandTimeDeltaWithJitterSetterForTesting scoped_rand_time_delta(
      base::Minutes(7));

  // Act
  base::test::TestFuture<bool> test_future_retry;
  database_table_.Retry(confirmation_queue_item.confirmation.transaction_id,
                        test_future_retry.GetCallback());
  EXPECT_TRUE(test_future_retry.Get());

  // Assert
  confirmation_queue_item.process_at = test::Now() + base::Minutes(7);
  confirmation_queue_item.retry_count = 1;
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((ConfirmationQueueItemList{confirmation_queue_item}), got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       RetryConfirmationQueueItemMultipleTimes) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation);
  ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  const ScopedRandTimeDeltaWithJitterSetterForTesting scoped_rand_time_delta(
      base::Minutes(7));

  {
    base::test::TestFuture<bool> test_future_retry;
    database_table_.Retry(confirmation_queue_item.confirmation.transaction_id,
                          test_future_retry.GetCallback());
    EXPECT_TRUE(test_future_retry.Get());
  }

  // Act
  base::test::TestFuture<bool> test_future_retry;
  database_table_.Retry(confirmation_queue_item.confirmation.transaction_id,
                        test_future_retry.GetCallback());
  EXPECT_TRUE(test_future_retry.Get());

  // Assert
  confirmation_queue_item.process_at = test::Now() + base::Minutes(14);
  confirmation_queue_item.retry_count = 2;
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((ConfirmationQueueItemList{confirmation_queue_item}), got_items);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableTest,
       DoNotRetryMissingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/3);

  ConfirmationQueueItemList confirmation_queue_items;

  std::optional<ConfirmationInfo> confirmation_1 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_1);
  ConfirmationQueueItemInfo confirmation_queue_item_1 =
      BuildConfirmationQueueItem(*confirmation_1, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_1);

  std::optional<ConfirmationInfo> confirmation_2 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_2);
  const ConfirmationQueueItemInfo confirmation_queue_item_2 =
      BuildConfirmationQueueItem(*confirmation_2, /*process_at=*/test::Now());
  confirmation_queue_items.push_back(confirmation_queue_item_2);

  test::SaveConfirmationQueueItems(confirmation_queue_items);

  std::optional<ConfirmationInfo> confirmation_3 =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/true);
  ASSERT_TRUE(confirmation_3);
  const ConfirmationQueueItemInfo confirmation_queue_item_3 =
      BuildConfirmationQueueItem(*confirmation_3, /*process_at=*/test::Now());

  // Act
  base::test::TestFuture<bool> test_future_retry;
  database_table_.Retry(confirmation_queue_item_3.confirmation.transaction_id,
                        test_future_retry.GetCallback());
  EXPECT_TRUE(test_future_retry.Get());

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, got_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(confirmation_queue_items, got_items);
}

}  // namespace brave_ads::database::table
