/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/database/database_balance_report.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=DatabaseBalanceReportTest.*

using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;

namespace ledger {
namespace database {

class DatabaseBalanceReportTest : public ::testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::string execute_script_;
  std::unique_ptr<DatabaseBalanceReport> balance_report_;

  DatabaseBalanceReportTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    balance_report_ =
        std::make_unique<DatabaseBalanceReport>(mock_ledger_impl_.get());
  }

  ~DatabaseBalanceReportTest() override {}
};

TEST_F(DatabaseBalanceReportTest, InsertOrUpdateOk) {
  auto info = type::BalanceReportInfo::New();
  info->id = "2020_05";
  info->grants = 1.0;
  info->earning_from_ads = 1.0;
  info->auto_contribute = 1.0;
  info->recurring_donation = 1.0;
  info->one_time_donation = 1.0;

  const std::string query =
      "INSERT OR REPLACE INTO balance_report_info "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, ?, ?, ?, ?, ?)";

  ON_CALL(*mock_ledger_client_,
          RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .WillByDefault(
        Invoke([&](
            type::DBTransactionPtr transaction,
            ledger::client::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              type::DBCommand::Type::RUN);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 6u);
        }));

  balance_report_->InsertOrUpdate(
      std::move(info),
      [](const type::Result){});
}

TEST_F(DatabaseBalanceReportTest, GetAllRecordsOk) {
  EXPECT_CALL(*mock_ledger_client_,
              RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .Times(1);

  const std::string query =
    "SELECT balance_report_id, grants_ugp, grants_ads, "
    "auto_contribute, tip_recurring, tip "
    "FROM balance_report_info";

  ON_CALL(*mock_ledger_client_,
          RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .WillByDefault(
        Invoke([&](
            type::DBTransactionPtr transaction,
            ledger::client::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              type::DBCommand::Type::READ);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 6u);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 0u);
        }));

  balance_report_->GetAllRecords([](type::BalanceReportInfoList) {});
}

TEST_F(DatabaseBalanceReportTest, GetRecordOk) {
  EXPECT_CALL(*mock_ledger_client_,
              RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .Times(1);

  const std::string query =
    "SELECT balance_report_id, grants_ugp, grants_ads, "
    "auto_contribute, tip_recurring, tip "
    "FROM balance_report_info "
    "WHERE balance_report_id = ?";

  ON_CALL(*mock_ledger_client_,
          RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .WillByDefault(
        Invoke([&](
            type::DBTransactionPtr transaction,
            ledger::client::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 2u);
          ASSERT_EQ(
              transaction->commands[1]->type,
              type::DBCommand::Type::READ);
          ASSERT_EQ(transaction->commands[1]->command, query);
          ASSERT_EQ(transaction->commands[1]->record_bindings.size(), 6u);
          ASSERT_EQ(transaction->commands[1]->bindings.size(), 1u);
        }));

  balance_report_->GetRecord(
      type::ActivityMonth::MAY,
      2020,
      [](type::Result, type::BalanceReportInfoPtr) {});
}

TEST_F(DatabaseBalanceReportTest, DeleteAllRecordsOk) {
  EXPECT_CALL(*mock_ledger_client_,
              RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .Times(1);

  const std::string query =
    "DELETE FROM balance_report_info";

  ON_CALL(*mock_ledger_client_,
          RunDBTransaction(_, Matcher<client::RunDBTransactionCallback>(_)))
      .WillByDefault(
        Invoke([&](
            type::DBTransactionPtr transaction,
            ledger::client::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              type::DBCommand::Type::EXECUTE);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 0u);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 0u);
        }));

  balance_report_->DeleteAllRecords([](type::Result) {});
}

}  // namespace database
}  // namespace ledger
