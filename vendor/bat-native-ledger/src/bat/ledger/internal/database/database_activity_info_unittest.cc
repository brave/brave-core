/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=DatabaseActivityInfoTest.*

using ::testing::_;
using ::testing::Invoke;

namespace braveledger_database {

class DatabaseActivityInfoTest : public ::testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::string execute_script_;
  std::unique_ptr<DatabaseActivityInfo> activity_;
  std::unique_ptr<braveledger_database::MockDatabase> mock_database_;

  DatabaseActivityInfoTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    activity_ = std::make_unique<DatabaseActivityInfo>(mock_ledger_impl_.get());
    mock_database_ = std::make_unique<braveledger_database::MockDatabase>(
        mock_ledger_impl_.get());
  }

  ~DatabaseActivityInfoTest() override {}

  void SetUp() override {
    ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(testing::Return(mock_database_.get()));
  }
};

TEST_F(DatabaseActivityInfoTest, InsertOrUpdateNull) {
  EXPECT_CALL(*mock_ledger_client_, RunDBTransaction(_, _)).Times(0);

  ledger::PublisherInfoList list;
  list.push_back(nullptr);

  activity_->InsertOrUpdate(nullptr, [](const ledger::Result){});
}

TEST_F(DatabaseActivityInfoTest, InsertOrUpdateOk) {
  auto info = ledger::PublisherInfo::New();
  info->id = "publisher_2";
  info->duration = 10;
  info->score = 1.1;
  info->percent = 100;
  info->reconcile_stamp = 0;
  info->visits = 1;

  const std::string query =
      "INSERT OR REPLACE INTO activity_info "
      "(publisher_id, duration, score, percent, "
      "weight, reconcile_stamp, visits) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)";

  ON_CALL(*mock_ledger_client_, RunDBTransaction(_, _))
      .WillByDefault(
        Invoke([&](
            ledger::DBTransactionPtr transaction,
            ledger::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              ledger::DBCommand::Type::RUN);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 7u);
        }));

  activity_->InsertOrUpdate(
      std::move(info),
      [](const ledger::Result){});
}

TEST_F(DatabaseActivityInfoTest, GetRecordsListNull) {
  EXPECT_CALL(*mock_ledger_client_, RunDBTransaction(_, _)).Times(0);

  activity_->GetRecordsList(0, 0, nullptr, [](ledger::PublisherInfoList){});
}

TEST_F(DatabaseActivityInfoTest, GetRecordsListEmpty) {
  EXPECT_CALL(*mock_ledger_client_, RunDBTransaction(_, _)).Times(1);

  const std::string query =
      "SELECT ai.publisher_id, ai.duration, ai.score, "
      "ai.percent, ai.weight, spi.status, spi.updated_at, pi.excluded, "
      "pi.name, pi.url, pi.provider, "
      "pi.favIcon, ai.reconcile_stamp, ai.visits "
      "FROM activity_info AS ai "
      "INNER JOIN publisher_info AS pi "
      "ON ai.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE 1 = 1 AND pi.excluded = ?";

  ON_CALL(*mock_ledger_client_, RunDBTransaction(_, _))
      .WillByDefault(
        Invoke([&](
            ledger::DBTransactionPtr transaction,
            ledger::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              ledger::DBCommand::Type::READ);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 14u);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 1u);
        }));

  auto filter = ledger::ActivityInfoFilter::New();

  activity_->GetRecordsList(
      0,
      0,
      std::move(filter),
      [](ledger::PublisherInfoList){});
}

TEST_F(DatabaseActivityInfoTest, GetRecordsListOk) {
  EXPECT_CALL(*mock_ledger_client_, RunDBTransaction(_, _)).Times(1);

  const std::string query =
      "SELECT ai.publisher_id, ai.duration, ai.score, "
      "ai.percent, ai.weight, spi.status, spi.updated_at, pi.excluded, "
      "pi.name, pi.url, pi.provider, "
      "pi.favIcon, ai.reconcile_stamp, ai.visits "
      "FROM activity_info AS ai "
      "INNER JOIN publisher_info AS pi "
      "ON ai.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE 1 = 1 AND ai.publisher_id = ? AND pi.excluded = ?";

  ON_CALL(*mock_ledger_client_, RunDBTransaction(_, _))
      .WillByDefault(
        Invoke([&](
            ledger::DBTransactionPtr transaction,
            ledger::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              ledger::DBCommand::Type::READ);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 14u);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 2u);
        }));

  auto filter = ledger::ActivityInfoFilter::New();
  filter->id = "publisher_key";

  activity_->GetRecordsList(
      0,
      0,
      std::move(filter),
      [](ledger::PublisherInfoList){});
}

TEST_F(DatabaseActivityInfoTest, DeleteRecordEmpty) {
  EXPECT_CALL(*mock_ledger_client_, RunDBTransaction(_, _)).Times(0);

  const std::string query =
      "DELETE FROM %s WHERE publisher_id = ? AND reconcile_stamp = ?";

  activity_->DeleteRecord("", [](const ledger::Result){});
}

TEST_F(DatabaseActivityInfoTest, DeleteRecordOk) {
  EXPECT_CALL(*mock_ledger_client_, RunDBTransaction(_, _)).Times(1);

  ON_CALL(*mock_ledger_client_, GetUint64State(_))
      .WillByDefault(testing::Return(1597744617));

  const std::string query =
      "DELETE FROM activity_info "
      "WHERE publisher_id = ? AND reconcile_stamp = ?";

  ON_CALL(*mock_ledger_client_, RunDBTransaction(_, _))
      .WillByDefault(
        Invoke([&](
            ledger::DBTransactionPtr transaction,
            ledger::RunDBTransactionCallback callback) {
          ASSERT_TRUE(transaction);
          ASSERT_EQ(transaction->commands.size(), 1u);
          ASSERT_EQ(
              transaction->commands[0]->type,
              ledger::DBCommand::Type::RUN);
          ASSERT_EQ(transaction->commands[0]->command, query);
          ASSERT_EQ(transaction->commands[0]->bindings.size(), 2u);
        }));

  activity_->DeleteRecord("publisher_key", [](const ledger::Result){});
}

}  // namespace braveledger_database
