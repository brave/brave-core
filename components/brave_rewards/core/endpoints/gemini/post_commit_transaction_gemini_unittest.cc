/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/gemini/post_commit_transaction_gemini.h"

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

using Error = endpoints::PostCommitTransactionGemini::Error;
using Result = endpoints::PostCommitTransactionGemini::Result;

using PostCommitTransactionGeminiParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;

class RewardsPostCommitTransactionGeminiTest
    : public RewardsEngineTest,
      public WithParamInterface<PostCommitTransactionGeminiParamType> {};

TEST_P(RewardsPostCommitTransactionGeminiTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().gemini_api_url().Resolve(
      "/v1/payments/pay");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::PostCommitTransactionGemini>(
        engine(), "token", "address",
        mojom::ExternalTransaction::New("transaction_id", "contribution_id",
                                        "destination", "amount"))
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostCommitTransactionGeminiTest,
    RewardsPostCommitTransactionGeminiTest,
    Values(
        PostCommitTransactionGeminiParamType{
            "HTTP_200_response_not_a_dict", net::HTTP_OK,
            R"(
        [
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "Pending",
          "tx_ref": "transaction_id"
        ]
      )",
            base::unexpected(Error::kFailedToParseBody)},
        PostCommitTransactionGeminiParamType{
            "HTTP_200_status_wrong_case", net::HTTP_OK,
            R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "STATUS": "Pending",
          "tx_ref": "transaction_id"
        }
      )",
            base::unexpected(Error::kFailedToParseBody)},
        PostCommitTransactionGeminiParamType{
            "HTTP_200_transaction_pending", net::HTTP_OK,
            R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "Pending",
          "tx_ref": "transaction_id"
        }
      )",
            base::unexpected(Error::kTransactionPending)},
        PostCommitTransactionGeminiParamType{
            "HTTP_200_unknown_status", net::HTTP_OK,
            R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "UnknownStatus",
          "tx_ref": "transaction_id"
        }
      )",
            base::unexpected(Error::kUnexpectedError)},
        PostCommitTransactionGeminiParamType{"HTTP_200_transaction_completed",
                                             net::HTTP_OK,
                                             R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "Completed",
          "tx_ref": "transaction_id"
        }
      )",
                                             {}},
        PostCommitTransactionGeminiParamType{
            "HTTP_401_access_token_expired", net::HTTP_UNAUTHORIZED, "",
            base::unexpected(Error::kAccessTokenExpired)},
        PostCommitTransactionGeminiParamType{
            "HTTP_500_unexpected_status_code", net::HTTP_INTERNAL_SERVER_ERROR,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
