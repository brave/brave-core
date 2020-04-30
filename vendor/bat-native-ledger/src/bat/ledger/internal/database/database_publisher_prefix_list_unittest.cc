/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/big_endian.h"
#include "base/test/task_environment.h"
#include "base/strings/string_piece.h"
#include "bat/ledger/internal/database/database_publisher_prefix_list.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/publisher/protos/publisher_prefix_list.pb.h"

// npm run test -- brave_unit_tests --filter='DatabasePublisherPrefixListTest.*'

using ::testing::_;
using ::testing::Invoke;
using braveledger_publisher::PrefixListReader;

namespace braveledger_database {

class DatabasePublisherPrefixListTest : public ::testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::string execute_script_;
  std::unique_ptr<DatabasePublisherPrefixList> database_prefix_list_;

  DatabasePublisherPrefixListTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    database_prefix_list_ = std::make_unique<DatabasePublisherPrefixList>(
        mock_ledger_impl_.get());
  }

  ~DatabasePublisherPrefixListTest() override {}

  std::unique_ptr<PrefixListReader> CreateReader(uint32_t prefix_count) {
    auto reader = std::make_unique<PrefixListReader>();
    if (prefix_count == 0) {
      return reader;
    }

    std::string prefixes;
    prefixes.resize(prefix_count * 4);
    for (uint32_t i = 0; i < prefix_count; ++i) {
      base::WriteBigEndian(&prefixes[i * 4], i);
    }

    publishers_pb::PublisherPrefixList message;
    message.set_prefix_size(4);
    message.set_compression_type(
        publishers_pb::PublisherPrefixList::NO_COMPRESSION);
    message.set_uncompressed_size(prefixes.size());
    message.set_prefixes(std::move(prefixes));

    std::string out;
    message.SerializeToString(&out);
    reader->Parse(out);
    return reader;
  }

  void ExpectStartsWith(
      const std::string& subject,
      const std::string& prefix) {
    EXPECT_EQ(
        subject.size() > prefix.size()
            ? subject.substr(0, prefix.size())
            : subject,
        prefix);
  }
};

TEST_F(DatabasePublisherPrefixListTest, Reset) {
  std::vector<std::string> commands;

  auto on_run_db_transaction = [&](
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback) {
    ASSERT_TRUE(transaction);
    if (transaction) {
      for (auto& command : transaction->commands) {
        commands.push_back(std::move(command->command));
      }
    }
    commands.push_back("---");
    auto response = ledger::DBCommandResponse::New();
    response->status = ledger::DBCommandResponse::Status::RESPONSE_OK;
    callback(std::move(response));
  };

  ON_CALL(*mock_ledger_impl_, RunDBTransaction(_, _))
      .WillByDefault(Invoke(on_run_db_transaction));

  database_prefix_list_->Reset(
      CreateReader(100'001),
      [](const ledger::Result) {});

  ASSERT_EQ(commands.size(), 5u);
  EXPECT_EQ(commands[0], "DELETE FROM publisher_prefix_list");
  ExpectStartsWith(commands[1],
      "INSERT OR REPLACE INTO publisher_prefix_list (hash_prefix) "
      "VALUES (x'00000000'),(x'00000001'),(x'00000002'),");
  EXPECT_EQ(commands[2], "---");
  EXPECT_EQ(commands[3],
      "INSERT OR REPLACE INTO publisher_prefix_list (hash_prefix) "
      "VALUES (x'000186A0')");
  EXPECT_EQ(commands[4], "---");
}

}  // namespace braveledger_database
