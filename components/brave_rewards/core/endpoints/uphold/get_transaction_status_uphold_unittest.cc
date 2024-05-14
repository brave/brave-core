/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/get_transaction_status_uphold.h"

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

namespace brave_rewards::internal {

using Error = endpoints::GetTransactionStatusUphold::Error;
using Result = endpoints::GetTransactionStatusUphold::Result;

using GetTransactionStatusUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // get transaction status endpoint status code
    std::string,          // get transaction status endpoint response body
    Result                // expected result
>;

class RewardsGetTransactionStatusUpholdTest
    : public RewardsEngineTest,
      public WithParamInterface<GetTransactionStatusUpholdParamType> {};

TEST_P(RewardsGetTransactionStatusUpholdTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
      "/v0/me/transactions/transaction_id");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(request_url.spec(), mojom::UrlMethod::GET,
                                      std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::GetTransactionStatusUphold>(
        engine(), "token", "transaction_id")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsGetTransactionStatusUpholdTest,
    RewardsGetTransactionStatusUpholdTest,
    Values(
        GetTransactionStatusUpholdParamType{
            "HTTP_200_response_not_a_dict", net::HTTP_OK,
            R"(
        [
          "status": "completed"
        ]
      )",
            base::unexpected(Error::kFailedToParseBody)},
        GetTransactionStatusUpholdParamType{
            "HTTP_200_status_wrong_case", net::HTTP_OK,
            R"(
        {
          "STATUS": "completed"
        }
      )",
            base::unexpected(Error::kFailedToParseBody)},
        GetTransactionStatusUpholdParamType{
            "HTTP_200_transaction_pending", net::HTTP_OK,
            R"(
        {
          "status": "processing"
        }
      )",
            base::unexpected(Error::kTransactionPending)},
        GetTransactionStatusUpholdParamType{
            "HTTP_200_unexpected_transaction_status", net::HTTP_OK,
            R"(
        {
          "status": "failed"
        }
      )",
            base::unexpected(Error::kUnexpectedTransactionStatus)},
        GetTransactionStatusUpholdParamType{"HTTP_200_transaction_completed",
                                            net::HTTP_OK,
                                            R"(
        {
          "status": "completed"
        }
      )",
                                            {}},
        GetTransactionStatusUpholdParamType{"HTTP_2xx_transaction_completed",
                                            net::HTTP_PARTIAL_CONTENT,
                                            R"(
        {
          "status": "completed"
        }
      )",
                                            {}},
        GetTransactionStatusUpholdParamType{
            "HTTP_401_access_token_expired", net::HTTP_UNAUTHORIZED, "",
            base::unexpected(Error::kAccessTokenExpired)},
        GetTransactionStatusUpholdParamType{
            "HTTP_500_unexpected_status_code", net::HTTP_INTERNAL_SERVER_ERROR,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
