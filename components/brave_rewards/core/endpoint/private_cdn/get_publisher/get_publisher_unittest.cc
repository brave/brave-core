/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/big_endian.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/private_cdn/get_publisher/get_publisher.h"
#include "brave/components/brave_rewards/core/endpoint/private_cdn/private_cdn_util.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/publisher/protos/channel_response.pb.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=GetPublisherTest.*

using ::testing::_;

namespace ledger::endpoint::private_cdn {

class GetPublisherTest : public testing::Test {
 protected:
  std::string StringifyChannelResponse(
      const publishers_pb::ChannelResponseList& message) {
    std::string out;
    message.SerializeToString(&out);

    // Add padding header
    uint32_t length = out.length();
    out.insert(0, 4, ' ');
    base::WriteBigEndian(&out[0], length);

    return out;
  }

  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  GetPublisher get_publisher{&mock_ledger_impl_};
};

TEST_F(GetPublisherTest, ServerError404) {
  EXPECT_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_NOT_FOUND;
        std::move(callback).Run(std::move(response));
      });

  get_publisher.Request(
      "brave.com", "ce55",
      [](mojom::Result result, mojom::ServerPublisherInfoPtr info) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->publisher_key, "brave.com");
        EXPECT_EQ(info->status, mojom::PublisherStatus::NOT_VERIFIED);
      });

  task_environment_.RunUntilIdle();
}

TEST_F(GetPublisherTest, UpholdVerified) {
  EXPECT_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .Times(1)
      .WillOnce([&](mojom::UrlRequestPtr request, auto callback) {
        publishers_pb::ChannelResponseList message;
        auto* channel = message.add_channel_responses();
        channel->set_channel_identifier("brave.com");

        auto* uphold_wallet = channel->add_wallets()->mutable_uphold_wallet();
        uphold_wallet->set_wallet_state(publishers_pb::UPHOLD_ACCOUNT_KYC);
        uphold_wallet->set_address("abcd");

        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = StringifyChannelResponse(message);
        std::move(callback).Run(std::move(response));
      });

  get_publisher.Request(
      "brave.com", "ce55",
      [](mojom::Result result, mojom::ServerPublisherInfoPtr info) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->publisher_key, "brave.com");
        EXPECT_EQ(info->status, mojom::PublisherStatus::UPHOLD_VERIFIED);
        EXPECT_EQ(info->address, "abcd");
      });

  task_environment_.RunUntilIdle();
}

TEST_F(GetPublisherTest, EmptyWalletAddress) {
  EXPECT_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .Times(1)
      .WillOnce([&](mojom::UrlRequestPtr request, auto callback) {
        publishers_pb::ChannelResponseList message;
        auto* channel = message.add_channel_responses();
        channel->set_channel_identifier("brave.com");

        auto* uphold_wallet = channel->add_wallets()->mutable_uphold_wallet();
        uphold_wallet->set_wallet_state(publishers_pb::UPHOLD_ACCOUNT_KYC);
        uphold_wallet->set_address("");

        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = StringifyChannelResponse(message);
        std::move(callback).Run(std::move(response));
      });

  get_publisher.Request(
      "brave.com", "ce55",
      [](mojom::Result result, mojom::ServerPublisherInfoPtr info) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->publisher_key, "brave.com");
        EXPECT_EQ(info->status, mojom::PublisherStatus::NOT_VERIFIED);
        EXPECT_EQ(info->address, "");
      });

  task_environment_.RunUntilIdle();
}

}  // namespace ledger::endpoint::private_cdn
