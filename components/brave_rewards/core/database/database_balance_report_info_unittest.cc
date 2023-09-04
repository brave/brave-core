/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/database/database_balance_report.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"

// npm run test -- brave_unit_tests --filter=DatabaseBalanceReportTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace database {

class DatabaseBalanceReportTest : public ::testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  DatabaseBalanceReport balance_report_{mock_engine_impl_};
};

TEST_F(DatabaseBalanceReportTest, InsertOrUpdateOk) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), RunDBTransaction(_, _))
      .Times(1)
      .WillOnce([](mojom::DBTransactionPtr transaction, auto callback) {
        ASSERT_TRUE(transaction);
        ASSERT_EQ(transaction->commands.size(), 1u);
        ASSERT_EQ(transaction->commands[0]->type, mojom::DBCommand::Type::RUN);
        const std::string query =
            "INSERT OR REPLACE INTO balance_report_info "
            "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
            "tip_recurring, tip) "
            "VALUES (?, ?, ?, ?, ?, ?)";
        ASSERT_EQ(transaction->commands[0]->command, query);
        ASSERT_EQ(transaction->commands[0]->bindings.size(), 6u);
        std::move(callback).Run(db_error_response->Clone());
      });

  auto info = mojom::BalanceReportInfo::New();
  info->id = "2020_05";
  info->grants = 1.0;
  info->earning_from_ads = 1.0;
  info->auto_contribute = 1.0;
  info->recurring_donation = 1.0;
  info->one_time_donation = 1.0;

  MockFunction<LegacyResultCallback> callback;
  EXPECT_CALL(callback, Call).Times(1);
  balance_report_.InsertOrUpdate(std::move(info), callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(DatabaseBalanceReportTest, GetAllRecordsOk) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), RunDBTransaction(_, _))
      .Times(1)
      .WillOnce([](mojom::DBTransactionPtr transaction, auto callback) {
        ASSERT_TRUE(transaction);
        ASSERT_EQ(transaction->commands.size(), 1u);
        ASSERT_EQ(transaction->commands[0]->type, mojom::DBCommand::Type::READ);
        const std::string query =
            "SELECT balance_report_id, grants_ugp, grants_ads, "
            "auto_contribute, tip_recurring, tip "
            "FROM balance_report_info";
        ASSERT_EQ(transaction->commands[0]->command, query);
        ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 6u);
        ASSERT_EQ(transaction->commands[0]->bindings.size(), 0u);
        std::move(callback).Run(db_error_response->Clone());
      });

  MockFunction<GetBalanceReportListCallback> callback;
  EXPECT_CALL(callback, Call).Times(1);
  balance_report_.GetAllRecords(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(DatabaseBalanceReportTest, GetRecordOk) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), RunDBTransaction(_, _))
      .Times(1)
      .WillOnce([](mojom::DBTransactionPtr transaction, auto callback) {
        ASSERT_TRUE(transaction);
        ASSERT_EQ(transaction->commands.size(), 2u);
        ASSERT_EQ(transaction->commands[1]->type, mojom::DBCommand::Type::READ);
        const std::string query =
            "SELECT balance_report_id, grants_ugp, grants_ads, "
            "auto_contribute, tip_recurring, tip "
            "FROM balance_report_info "
            "WHERE balance_report_id = ?";
        ASSERT_EQ(transaction->commands[1]->command, query);
        ASSERT_EQ(transaction->commands[1]->record_bindings.size(), 6u);
        ASSERT_EQ(transaction->commands[1]->bindings.size(), 1u);
        std::move(callback).Run(db_error_response->Clone());
      });

  base::MockCallback<mojom::RewardsEngine::GetBalanceReportCallback> callback;
  EXPECT_CALL(callback, Run).Times(1);
  balance_report_.GetRecord(mojom::ActivityMonth::MAY, 2020, callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(DatabaseBalanceReportTest, DeleteAllRecordsOk) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), RunDBTransaction(_, _))
      .Times(1)
      .WillOnce([](mojom::DBTransactionPtr transaction, auto callback) {
        ASSERT_TRUE(transaction);
        ASSERT_EQ(transaction->commands.size(), 1u);
        ASSERT_EQ(transaction->commands[0]->type,
                  mojom::DBCommand::Type::EXECUTE);
        const std::string query = "DELETE FROM balance_report_info";
        ASSERT_EQ(transaction->commands[0]->command, query);
        ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 0u);
        ASSERT_EQ(transaction->commands[0]->bindings.size(), 0u);
        std::move(callback).Run(db_error_response->Clone());
      });

  MockFunction<LegacyResultCallback> callback;
  EXPECT_CALL(callback, Call).Times(1);
  balance_report_.DeleteAllRecords(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace database
}  // namespace brave_rewards::internal
