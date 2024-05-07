/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_create_transaction_uphold.h"

#include <string>
#include <tuple>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal::endpoints {

using Error = endpoints::PostCreateTransactionUphold::Error;
using Result = endpoints::PostCreateTransactionUphold::Result;

using PostCreateTransactionUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create transaction endpoint status code
    std::string,          // post create transaction endpoint response body
    Result                // expected result
>;

class RewardsPostCreateTransactionUpholdTest
    : public RewardsEngineTest,
      public WithParamInterface<PostCreateTransactionUpholdParamType> {};

TEST_P(RewardsPostCreateTransactionUpholdTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
      "/v0/me/cards/address/transactions");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::PostCreateTransactionUphold>(
        engine(), "token", "address",
        mojom::ExternalTransaction::New("", "contribution_id", "destination",
                                        "amount"))
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostCreateTransactionUpholdTest,
    RewardsPostCreateTransactionUpholdTest,
    Values(
        PostCreateTransactionUpholdParamType{
            "HTTP_202_response_not_a_dict", net::HTTP_ACCEPTED,
            R"(
        [
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        ]
      )",
            base::unexpected(Error::kFailedToParseBody)},
        PostCreateTransactionUpholdParamType{
            "HTTP_202_id_wrong_case", net::HTTP_ACCEPTED,
            R"(
        {
          "ID": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
            base::unexpected(Error::kFailedToParseBody)},
        PostCreateTransactionUpholdParamType{
            "HTTP_202_ok", net::HTTP_ACCEPTED,
            R"(
        {
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
            "87725361-4245-4435-a75a-f7a85674714a"},
        PostCreateTransactionUpholdParamType{
            "HTTP_2xx_ok", net::HTTP_PARTIAL_CONTENT,
            R"(
        {
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
            "87725361-4245-4435-a75a-f7a85674714a"},
        PostCreateTransactionUpholdParamType{
            "HTTP_401_access_token_expired", net::HTTP_UNAUTHORIZED, "",
            base::unexpected(Error::kAccessTokenExpired)},
        PostCreateTransactionUpholdParamType{
            "HTTP_500_unexpected_status_code", net::HTTP_INTERNAL_SERVER_ERROR,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal::endpoints
