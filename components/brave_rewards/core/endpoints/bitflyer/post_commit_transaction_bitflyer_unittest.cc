/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/bitflyer/post_commit_transaction_bitflyer.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostCommitTransactionBitFlyer*

using ::testing::_;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostCommitTransactionBitFlyer::Error;
using Result = PostCommitTransactionBitFlyer::Result;

// clang-format off
using PostCommitTransactionBitFlyerParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;
// clang-format on

class PostCommitTransactionBitFlyer
    : public TestWithParam<PostCommitTransactionBitFlyerParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostCommitTransactionBitFlyer, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([&](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = status_code;
        response->body = body;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<base::OnceCallback<void(Result&&)>> callback;
  EXPECT_CALL(callback, Run(Result(expected_result))).Times(1);

  RequestFor<endpoints::PostCommitTransactionBitFlyer>(
      mock_engine_impl_, "token", "address",
      mojom::ExternalTransaction::New("transaction_id", "contribution_id",
                                      "destination", "amount"))
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostCommitTransactionBitFlyer,
  Values(
    PostCommitTransactionBitFlyerParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      "",
      {}
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_response_not_a_dict",
      net::HTTP_CONFLICT,
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
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_transfer_status_wrong_case",
      net::HTTP_CONFLICT,
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
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_access_token_expired",
      net::HTTP_CONFLICT,
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
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_unexpected_error",
      net::HTTP_CONFLICT,
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
      base::unexpected(Error::kUnexpectedError)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_500_unexpected_status_code",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
