/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/big_endian.h"
#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/endpoint/private_cdn/get_publisher/get_publisher.h"
#include "bat/ledger/internal/endpoint/private_cdn/private_cdn_util.h"
#include "bat/ledger/internal/publisher/protos/channel_response.pb.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=GetPublisherTest.*

namespace ledger {
namespace endpoint {
namespace private_cdn {

class GetPublisherTest : public BATLedgerTest {
 protected:
  mojom::Result Request(const std::string& id,
                        const std::string& prefix,
                        mojom::ServerPublisherInfoPtr* info) {
    DCHECK(info);
    base::RunLoop run_loop;
    mojom::Result result;

    GetPublisher(GetLedgerImpl())
        .Request(id, prefix,
                 [&run_loop, &result, info](
                     mojom::Result request_result,
                     mojom::ServerPublisherInfoPtr request_info) {
                   result = request_result;
                   *info = std::move(request_info);
                   run_loop.Quit();
                 });

    run_loop.Run();
    return result;
  }

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
};

TEST_F(GetPublisherTest, ServerError404) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HTTP_NOT_FOUND;
  AddNetworkResultForTesting(GetServerUrl("/publishers/prefixes/ce55"),
                             mojom::UrlMethod::GET, std::move(response));

  mojom::Result result;
  mojom::ServerPublisherInfoPtr info;

  result = Request("brave.com", "ce55", &info);
  EXPECT_EQ(result, mojom::Result::LEDGER_OK);
  ASSERT_TRUE(info);
  EXPECT_EQ(info->publisher_key, "brave.com");
  EXPECT_EQ(info->status, mojom::PublisherStatus::NOT_VERIFIED);
}

TEST_F(GetPublisherTest, UpholdVerified) {
  publishers_pb::ChannelResponseList message;
  auto* channel = message.add_channel_responses();
  channel->set_channel_identifier("brave.com");

  auto* uphold_wallet = channel->add_wallets()->mutable_uphold_wallet();
  uphold_wallet->set_wallet_state(publishers_pb::UPHOLD_ACCOUNT_KYC);
  uphold_wallet->set_address("abcd");

  auto response = mojom::UrlResponse::New();
  response->status_code = net::HTTP_OK;
  response->body = StringifyChannelResponse(message);

  AddNetworkResultForTesting(GetServerUrl("/publishers/prefixes/ce55"),
                             mojom::UrlMethod::GET, std::move(response));

  mojom::Result result;
  mojom::ServerPublisherInfoPtr info;

  result = Request("brave.com", "ce55", &info);
  EXPECT_EQ(result, mojom::Result::LEDGER_OK);
  ASSERT_TRUE(info);
  EXPECT_EQ(info->publisher_key, "brave.com");
  EXPECT_EQ(info->status, mojom::PublisherStatus::UPHOLD_VERIFIED);
  EXPECT_EQ(info->address, "abcd");
}

TEST_F(GetPublisherTest, EmptyWalletAddress) {
  publishers_pb::ChannelResponseList message;
  auto* channel = message.add_channel_responses();
  channel->set_channel_identifier("brave.com");

  auto* uphold_wallet = channel->add_wallets()->mutable_uphold_wallet();
  uphold_wallet->set_wallet_state(publishers_pb::UPHOLD_ACCOUNT_KYC);
  uphold_wallet->set_address("");

  auto response = mojom::UrlResponse::New();
  response->status_code = net::HTTP_OK;
  response->body = StringifyChannelResponse(message);

  AddNetworkResultForTesting(GetServerUrl("/publishers/prefixes/ce55"),
                             mojom::UrlMethod::GET, std::move(response));

  mojom::Result result;
  mojom::ServerPublisherInfoPtr info;

  result = Request("brave.com", "ce55", &info);
  EXPECT_EQ(result, mojom::Result::LEDGER_OK);
  ASSERT_TRUE(info);
  EXPECT_EQ(info->publisher_key, "brave.com");
  EXPECT_EQ(info->status, mojom::PublisherStatus::NOT_VERIFIED);
  EXPECT_EQ(info->address, "");
}

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger
