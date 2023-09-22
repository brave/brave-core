/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/big_endian.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/database/database_publisher_prefix_list.h"
#include "brave/components/brave_rewards/core/publisher/protos/publisher_prefix_list.pb.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"

// npm run test -- brave_unit_tests --filter=DatabasePublisherPrefixListTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace database {

class DatabasePublisherPrefixListTest : public ::testing::Test {
 protected:
  publisher::PrefixListReader CreateReader(uint32_t prefix_count) {
    publisher::PrefixListReader reader;
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
    reader.Parse(out);
    return reader;
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  DatabasePublisherPrefixList database_prefix_list_{mock_engine_impl_};
};

TEST_F(DatabasePublisherPrefixListTest, Reset) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), RunDBTransaction(_, _))
      .Times(2)
      .WillOnce([](mojom::DBTransactionPtr transaction, auto callback) {
        EXPECT_TRUE(transaction);
        EXPECT_EQ(transaction->commands.size(), 2u);
        EXPECT_EQ(transaction->commands[0]->command,
                  "DELETE FROM publisher_prefix_list");
        EXPECT_TRUE(base::StartsWith(
            transaction->commands[1]->command,
            "INSERT OR REPLACE INTO publisher_prefix_list (hash_prefix) "
            "VALUES (x'00000000'),(x'00000001'),(x'00000002'),"));

        auto response = mojom::DBCommandResponse::New();
        response->status = mojom::DBCommandResponse::Status::RESPONSE_OK;
        std::move(callback).Run(std::move(response));
      })
      .WillOnce([](mojom::DBTransactionPtr transaction, auto callback) {
        EXPECT_TRUE(transaction);
        EXPECT_EQ(transaction->commands.size(), 1u);
        EXPECT_EQ(transaction->commands[0]->command,
                  "INSERT OR REPLACE INTO publisher_prefix_list (hash_prefix) "
                  "VALUES (x'000186A0')");

        std::move(callback).Run(db_error_response->Clone());
      });

  MockFunction<LegacyResultCallback> callback;
  EXPECT_CALL(callback, Call).Times(1);
  database_prefix_list_.Reset(CreateReader(100'001), callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace database
}  // namespace brave_rewards::internal
