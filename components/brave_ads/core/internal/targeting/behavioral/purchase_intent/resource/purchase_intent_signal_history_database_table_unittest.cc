/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsPurchaseIntentSignalHistoryDatabaseTableTest
    : public test::TestBase {
 protected:
  PurchaseIntentSignalHistory database_table_;
};

TEST_F(BraveAdsPurchaseIntentSignalHistoryDatabaseTableTest,
       SaveEmptyPurchaseIntentSignalHistory) {
  // Act
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({}, save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PurchaseIntentSignalHistoryMap>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const PurchaseIntentSignalHistoryMap&>());
  const auto [success, purchase_intent_signal_history] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsPurchaseIntentSignalHistoryDatabaseTableTest,
       SavePurchaseIntentSignalHistory) {
  // Arrange
  const PurchaseIntentSignalHistoryMap purchase_intent_signal_history = {
      {"technology & computing",
       {{/*at=*/base::Time::Now(), /*weight=*/1},
        {/*at=*/base::Time::Now(), /*weight=*/2}}},
      {"travel", {{/*at=*/base::Time::Now(), /*weight=*/3}}}};

  // Act
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save(purchase_intent_signal_history,
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PurchaseIntentSignalHistoryMap>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const PurchaseIntentSignalHistoryMap&>());
  const auto [success, actual_purchase_intent_signal_history] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(purchase_intent_signal_history,
            actual_purchase_intent_signal_history);
}

TEST_F(BraveAdsPurchaseIntentSignalHistoryDatabaseTableTest,
       SaveForSegmentReplacesOnlyThatSegment) {
  // Arrange
  const base::Time now = base::Time::Now();

  const PurchaseIntentSignalHistoryList unaffected_history = {
      {/*at=*/now, /*weight=*/1}};

  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({{"technology & computing", unaffected_history},
                        {"travel", {{/*at=*/now, /*weight=*/2}}}},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  const PurchaseIntentSignalHistoryList new_history = {
      {/*at=*/now, /*weight=*/3}};
  base::test::TestFuture<bool> save_for_segment_test_future;
  database_table_.SaveForSegment("travel", new_history,
                                 save_for_segment_test_future.GetCallback());
  ASSERT_TRUE(save_for_segment_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PurchaseIntentSignalHistoryMap>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const PurchaseIntentSignalHistoryMap&>());
  const auto [success, purchase_intent_signal_history] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ((PurchaseIntentSignalHistoryMap{
                {"technology & computing", unaffected_history},
                {"travel", new_history}}),
            purchase_intent_signal_history);
}

TEST_F(BraveAdsPurchaseIntentSignalHistoryDatabaseTableTest,
       SaveForSegmentWithEmptyHistoryDeletesSegment) {
  // Arrange
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({{"travel", {{/*at=*/base::Time::Now(), /*weight=*/1}}}},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> save_for_segment_test_future;
  database_table_.SaveForSegment("travel", /*history=*/{},
                                 save_for_segment_test_future.GetCallback());
  ASSERT_TRUE(save_for_segment_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PurchaseIntentSignalHistoryMap>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const PurchaseIntentSignalHistoryMap&>());
  const auto [success, purchase_intent_signal_history] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsPurchaseIntentSignalHistoryDatabaseTableTest,
       DeleteAllPurchaseIntentSignalHistory) {
  // Arrange
  base::test::TestFuture<bool> save_test_future;
  database_table_.Save({{"travel", {{/*at=*/base::Time::Now(), /*weight=*/1}}}},
                       save_test_future.GetCallback());
  ASSERT_TRUE(save_test_future.Take());

  // Act
  base::test::TestFuture<bool> delete_all_test_future;
  database_table_.DeleteAll(delete_all_test_future.GetCallback());
  ASSERT_TRUE(delete_all_test_future.Take());

  // Assert
  base::test::TestFuture<bool, PurchaseIntentSignalHistoryMap>
      get_all_test_future;
  database_table_.GetAll(
      get_all_test_future
          .GetCallback<bool, const PurchaseIntentSignalHistoryMap&>());
  const auto [success, purchase_intent_signal_history] =
      get_all_test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

}  // namespace brave_ads::database::table
