/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_commit_transaction_uphold.h"

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

using Error = endpoints::PostCommitTransactionUphold::Error;
using Result = endpoints::PostCommitTransactionUphold::Result;

using PostCommitTransactionUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;

class RewardsPostCommitTransactionUpholdTest
    : public RewardsEngineTest,
      public WithParamInterface<PostCommitTransactionUpholdParamType> {};

TEST_P(RewardsPostCommitTransactionUpholdTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
      "/v0/me/cards/address/transactions/transaction_id/commit");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::PostCommitTransactionUphold>(
        engine(), "token", "address",
        mojom::ExternalTransaction::New("transaction_id", "contribution_id",
                                        "destination", "amount"))
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostCommitTransactionUpholdTest,
    RewardsPostCommitTransactionUpholdTest,
    Values(
        PostCommitTransactionUpholdParamType{
            "HTTP_200_response_not_a_dict", net::HTTP_OK,
            R"(
        [
          "status": "completed"
        ]
      )",
            base::unexpected(Error::kFailedToParseBody)},
        PostCommitTransactionUpholdParamType{
            "HTTP_200_status_wrong_case", net::HTTP_OK,
            R"(
        {
          "STATUS": "completed"
        }
      )",
            base::unexpected(Error::kFailedToParseBody)},
        PostCommitTransactionUpholdParamType{
            "HTTP_200_transaction_pending", net::HTTP_OK,
            R"(
        {
          "status": "processing"
        }
      )",
            base::unexpected(Error::kTransactionPending)},
        PostCommitTransactionUpholdParamType{
            "HTTP_200_unexpected_transaction_status", net::HTTP_OK,
            R"(
        {
          "status": "failed"
        }
      )",
            base::unexpected(Error::kUnexpectedTransactionStatus)},
        PostCommitTransactionUpholdParamType{"HTTP_200_transaction_completed",
                                             net::HTTP_OK,
                                             R"(
        {
          "status": "completed"
        }
      )",
                                             {}},
        PostCommitTransactionUpholdParamType{"HTTP_2xx_transaction_completed",
                                             net::HTTP_PARTIAL_CONTENT,
                                             R"(
        {
          "status": "completed"
        }
      )",
                                             {}},
        PostCommitTransactionUpholdParamType{
            "HTTP_401_access_token_expired", net::HTTP_UNAUTHORIZED, "",
            base::unexpected(Error::kAccessTokenExpired)},
        PostCommitTransactionUpholdParamType{
            "HTTP_404_transaction_not_found", net::HTTP_NOT_FOUND, "",
            base::unexpected(Error::kTransactionNotFound)},
        PostCommitTransactionUpholdParamType{
            "HTTP_500_unexpected_status_code", net::HTTP_INTERNAL_SERVER_ERROR,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
