/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/bitflyer/post_commit_transaction_bitflyer.h"

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

using Error = endpoints::PostCommitTransactionBitFlyer::Error;
using Result = endpoints::PostCommitTransactionBitFlyer::Result;

using PostCommitTransactionBitFlyerParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;

class RewardsPostCommitTransactionBitFlyerTest
    : public RewardsEngineTest,
      public WithParamInterface<PostCommitTransactionBitFlyerParamType> {};

TEST_P(RewardsPostCommitTransactionBitFlyerTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().bitflyer_url().Resolve(
      "/api/link/v1/coin/withdraw-to-deposit-id/request");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::PostCommitTransactionBitFlyer>(
        engine(), "token", "address",
        mojom::ExternalTransaction::New("transaction_id", "contribution_id",
                                        "destination", "amount"))
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostCommitTransactionBitFlyerTest,
    RewardsPostCommitTransactionBitFlyerTest,
    Values(PostCommitTransactionBitFlyerParamType{"HTTP_200_success",
                                                  net::HTTP_OK,
                                                  "",
                                                  {}},
           PostCommitTransactionBitFlyerParamType{
               "HTTP_401_access_token_expired", net::HTTP_UNAUTHORIZED, "",
               base::unexpected(Error::kAccessTokenExpired)},
           PostCommitTransactionBitFlyerParamType{
               "HTTP_409_response_not_a_dict", net::HTTP_CONFLICT,
               R"(
        [
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "transfer_status": "SESSION_TIME_OUT"
        ]
      )",
               base::unexpected(Error::kFailedToParseBody)},
           PostCommitTransactionBitFlyerParamType{
               "HTTP_409_transfer_status_wrong_case", net::HTTP_CONFLICT,
               R"(
        {
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "TRANSFER_STATUS": "SESSION_TIME_OUT"
        }
      )",
               base::unexpected(Error::kFailedToParseBody)},
           PostCommitTransactionBitFlyerParamType{
               "HTTP_409_access_token_expired", net::HTTP_CONFLICT,
               R"(
        {
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "transfer_status": "SESSION_TIME_OUT"
        }
      )",
               base::unexpected(Error::kAccessTokenExpired)},
           PostCommitTransactionBitFlyerParamType{
               "HTTP_409_unexpected_error", net::HTTP_CONFLICT,
               R"(
        {
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "transfer_status": "NOT_ALLOWED_TO_SEND"
        }
      )",
               base::unexpected(Error::kUnexpectedError)},
           PostCommitTransactionBitFlyerParamType{
               "HTTP_500_unexpected_status_code",
               net::HTTP_INTERNAL_SERVER_ERROR, "",
               base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
