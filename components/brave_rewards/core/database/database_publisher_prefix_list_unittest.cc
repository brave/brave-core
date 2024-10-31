/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/database/database_publisher_prefix_list.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/byte_conversions.h"
#include "brave/components/brave_rewards/core/publisher/protos/publisher_prefix_list.pb.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

namespace {

class Client : public TestRewardsEngineClient {
 public:
  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) override {
    transactions_.push_back(transaction->Clone());
    auto response = mojom::DBCommandResponse::New();
    response->status = mojom::DBCommandResponse::Status::RESPONSE_OK;
    std::move(callback).Run(std::move(response));
  }

  std::vector<mojom::DBTransactionPtr>& transactions() { return transactions_; }

 private:
  std::vector<mojom::DBTransactionPtr> transactions_;
};

}  // namespace

class RewardsDatabasePublisherPrefixListTest : public RewardsEngineTest {
 protected:
  RewardsDatabasePublisherPrefixListTest()
      : RewardsEngineTest(std::make_unique<Client>()) {}

  Client& client() { return static_cast<Client&>(RewardsEngineTest::client()); }

  publisher::PrefixListReader CreateReader(uint32_t prefix_count) {
    publisher::PrefixListReader reader;
    if (prefix_count == 0) {
      return reader;
    }

    std::string prefixes;
    prefixes.resize(prefix_count * 4);
    for (uint32_t i = 0; i < prefix_count; ++i) {
      base::as_writable_byte_span(prefixes).subspan(i * 4).first<4>().copy_from(
          base::numerics::U32ToBigEndian(i));
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
};

TEST_F(RewardsDatabasePublisherPrefixListTest, Reset) {
  database::DatabasePublisherPrefixList prefix_list(engine());
  WaitFor<mojom::Result>([&](auto callback) {
    prefix_list.Reset(CreateReader(100'001), std::move(callback));
  });

  auto& transactions = client().transactions();
  ASSERT_EQ(transactions.size(), 2ul);

  {
    auto& transaction = transactions[0];
    EXPECT_TRUE(transaction);
    EXPECT_EQ(transaction->commands.size(), 2u);
    EXPECT_EQ(transaction->commands[0]->command,
              "DELETE FROM publisher_prefix_list");
    EXPECT_TRUE(transaction->commands[1]->command.starts_with(
        "INSERT OR REPLACE INTO publisher_prefix_list (hash_prefix) "
        "VALUES (x'00000000'),(x'00000001'),(x'00000002'),"));
  }

  {
    auto& transaction = transactions[1];
    EXPECT_TRUE(transaction);
    EXPECT_EQ(transaction->commands.size(), 1u);
    EXPECT_EQ(transaction->commands[0]->command,
              "INSERT OR REPLACE INTO publisher_prefix_list (hash_prefix) "
              "VALUES (x'000186A0')");
  }
}

}  // namespace brave_rewards::internal
