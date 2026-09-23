/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_page_probabilities_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

std::vector<base::Time> GetAllCreatedAtValues() {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql =
      "SELECT created_at FROM text_classification_page_probabilities "
      "ORDER BY created_at DESC";
  mojom_db_action->bind_column_types = {mojom::DBBindColumnType::kTime};
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  base::test::TestFuture<mojom::DBTransactionResultInfoPtr> test_future;
  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 test_future.GetCallback());
  mojom::DBTransactionResultInfoPtr mojom_db_transaction_result =
      test_future.Take();
  CHECK(IsTransactionSuccessful(mojom_db_transaction_result));
  CHECK(mojom_db_transaction_result->rows_union);

  std::vector<base::Time> created_at_values;
  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    created_at_values.push_back(ColumnTime(mojom_db_row, 0));
  }
  return created_at_values;
}

bool RunTransactionForTest(mojom::DBTransactionInfoPtr mojom_db_transaction) {
  base::test::TestFuture<bool> test_future;
  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 test_future.GetCallback());
  return test_future.Take();
}

}  // namespace

class BraveAdsTextClassificationPageProbabilitiesDatabaseTableTest
    : public test::TestBase {
 protected:
  TextClassificationPageProbabilities database_table_;
};

TEST_F(BraveAdsTextClassificationPageProbabilitiesDatabaseTableTest, Insert) {
  // Arrange
  const base::Time created_at = base::Time::Now();

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  database_table_.Insert(mojom_db_transaction, created_at);

  // Act
  ASSERT_TRUE(RunTransactionForTest(std::move(mojom_db_transaction)));

  // Assert
  EXPECT_THAT(GetAllCreatedAtValues(), ::testing::ElementsAre(created_at));
}

TEST_F(BraveAdsTextClassificationPageProbabilitiesDatabaseTableTest,
       InsertingTwiceKeepsBothEntries) {
  // Arrange
  const base::Time oldest_created_at = base::Time::Now();
  const base::Time newest_created_at = oldest_created_at + base::Seconds(1);

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  database_table_.Insert(mojom_db_transaction, oldest_created_at);
  database_table_.Insert(mojom_db_transaction, newest_created_at);

  // Act
  ASSERT_TRUE(RunTransactionForTest(std::move(mojom_db_transaction)));

  // Assert
  EXPECT_THAT(GetAllCreatedAtValues(),
              ::testing::ElementsAre(newest_created_at, oldest_created_at));
}

TEST_F(BraveAdsTextClassificationPageProbabilitiesDatabaseTableTest,
       DeleteAll) {
  // Arrange
  mojom::DBTransactionInfoPtr insert_mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  database_table_.Insert(insert_mojom_db_transaction, base::Time::Now());
  ASSERT_TRUE(RunTransactionForTest(std::move(insert_mojom_db_transaction)));

  mojom::DBTransactionInfoPtr delete_mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  database_table_.DeleteAll(delete_mojom_db_transaction);

  // Act
  ASSERT_TRUE(RunTransactionForTest(std::move(delete_mojom_db_transaction)));

  // Assert
  EXPECT_THAT(GetAllCreatedAtValues(), ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationPageProbabilitiesDatabaseTableTest,
       PruneToMaximumEntriesKeepsOnlyTheNewestEntries) {
  // Arrange
  const base::Time now = base::Time::Now();

  mojom::DBTransactionInfoPtr insert_mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  for (int i = 0; i < 3; ++i) {
    database_table_.Insert(insert_mojom_db_transaction, now + base::Seconds(i));
  }
  ASSERT_TRUE(RunTransactionForTest(std::move(insert_mojom_db_transaction)));

  mojom::DBTransactionInfoPtr prune_mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  database_table_.PruneToMaximumEntries(prune_mojom_db_transaction,
                                        /*maximum_entries=*/2);

  // Act
  ASSERT_TRUE(RunTransactionForTest(std::move(prune_mojom_db_transaction)));

  // Assert
  EXPECT_THAT(
      GetAllCreatedAtValues(),
      ::testing::ElementsAre(now + base::Seconds(2), now + base::Seconds(1)));
}

}  // namespace brave_ads::database::table
