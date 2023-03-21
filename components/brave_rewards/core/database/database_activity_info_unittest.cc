/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/database/database_activity_info.h"
#include "brave/components/brave_rewards/core/database/database_mock.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=DatabaseActivityInfoTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace database {

class DatabaseActivityInfoTest : public ::testing::Test {
  void SetUp() override {
    ON_CALL(mock_ledger_impl_, database())
        .WillByDefault(testing::Return(&mock_database_));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  DatabaseActivityInfo activity_{&mock_ledger_impl_};
  database::MockDatabase mock_database_{&mock_ledger_impl_};
};

TEST_F(DatabaseActivityInfoTest, InsertOrUpdateNull) {
  EXPECT_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .Times(0);

  std::vector<mojom::PublisherInfoPtr> list;
  list.push_back(nullptr);

  activity_.InsertOrUpdate(nullptr, [](const mojom::Result) {});
}

TEST_F(DatabaseActivityInfoTest, InsertOrUpdateOk) {
  auto info = mojom::PublisherInfo::New();
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

  ON_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .WillByDefault(
          Invoke([&](mojom::DBTransactionPtr transaction,
                     ledger::client::RunDBTransactionCallback callback) {
            ASSERT_TRUE(transaction);
            ASSERT_EQ(transaction->commands.size(), 1u);
            ASSERT_EQ(transaction->commands[0]->type,
                      mojom::DBCommand::Type::RUN);
            ASSERT_EQ(transaction->commands[0]->command, query);
            ASSERT_EQ(transaction->commands[0]->bindings.size(), 7u);
          }));

  activity_.InsertOrUpdate(std::move(info), [](const mojom::Result) {});
}

TEST_F(DatabaseActivityInfoTest, GetRecordsListNull) {
  EXPECT_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .Times(0);

  activity_.GetRecordsList(0, 0, nullptr,
                           [](std::vector<mojom::PublisherInfoPtr>) {});
}

TEST_F(DatabaseActivityInfoTest, GetRecordsListEmpty) {
  EXPECT_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .Times(1);

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

  ON_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .WillByDefault(
          Invoke([&](mojom::DBTransactionPtr transaction,
                     ledger::client::RunDBTransactionCallback callback) {
            ASSERT_TRUE(transaction);
            ASSERT_EQ(transaction->commands.size(), 1u);
            ASSERT_EQ(transaction->commands[0]->type,
                      mojom::DBCommand::Type::READ);
            ASSERT_EQ(transaction->commands[0]->command, query);
            ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 14u);
            ASSERT_EQ(transaction->commands[0]->bindings.size(), 1u);
          }));

  auto filter = mojom::ActivityInfoFilter::New();

  activity_.GetRecordsList(0, 0, std::move(filter),
                           [](std::vector<mojom::PublisherInfoPtr>) {});
}

TEST_F(DatabaseActivityInfoTest, GetRecordsListOk) {
  EXPECT_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .Times(1);

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

  ON_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .WillByDefault(
          Invoke([&](mojom::DBTransactionPtr transaction,
                     ledger::client::RunDBTransactionCallback callback) {
            ASSERT_TRUE(transaction);
            ASSERT_EQ(transaction->commands.size(), 1u);
            ASSERT_EQ(transaction->commands[0]->type,
                      mojom::DBCommand::Type::READ);
            ASSERT_EQ(transaction->commands[0]->command, query);
            ASSERT_EQ(transaction->commands[0]->record_bindings.size(), 14u);
            ASSERT_EQ(transaction->commands[0]->bindings.size(), 2u);
          }));

  auto filter = mojom::ActivityInfoFilter::New();
  filter->id = "publisher_key";

  activity_.GetRecordsList(0, 0, std::move(filter),
                           [](std::vector<mojom::PublisherInfoPtr>) {});
}

TEST_F(DatabaseActivityInfoTest, DeleteRecordEmpty) {
  EXPECT_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .Times(0);

  const std::string query =
      "DELETE FROM %s WHERE publisher_id = ? AND reconcile_stamp = ?";

  activity_.DeleteRecord("", [](const mojom::Result) {});
}

TEST_F(DatabaseActivityInfoTest, DeleteRecordOk) {
  EXPECT_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .Times(1);

  ON_CALL(*mock_ledger_impl_.ledger_client(), GetUint64State(_))
      .WillByDefault(testing::Return(1597744617));

  const std::string query =
      "DELETE FROM activity_info "
      "WHERE publisher_id = ? AND reconcile_stamp = ?";

  ON_CALL(*mock_ledger_impl_.ledger_client(), RunDBTransaction(_, _))
      .WillByDefault(
          Invoke([&](mojom::DBTransactionPtr transaction,
                     ledger::client::RunDBTransactionCallback callback) {
            ASSERT_TRUE(transaction);
            ASSERT_EQ(transaction->commands.size(), 1u);
            ASSERT_EQ(transaction->commands[0]->type,
                      mojom::DBCommand::Type::RUN);
            ASSERT_EQ(transaction->commands[0]->command, query);
            ASSERT_EQ(transaction->commands[0]->bindings.size(), 2u);
          }));

  activity_.DeleteRecord("publisher_key", [](const mojom::Result) {});
}

}  // namespace database
}  // namespace ledger
