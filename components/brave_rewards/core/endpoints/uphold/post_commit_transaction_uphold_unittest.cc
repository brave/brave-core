/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/endpoints/uphold/post_commit_transaction_uphold.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostCommitTransactionUphold*

using ::testing::_;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostCommitTransactionUphold::Error;
using Result = PostCommitTransactionUphold::Result;

// clang-format off
using PostCommitTransactionUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;
// clang-format on

class PostCommitTransactionUphold
    : public TestWithParam<PostCommitTransactionUpholdParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostCommitTransactionUphold, Paths) {
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

  RequestFor<endpoints::PostCommitTransactionUphold>(
      mock_engine_impl_, "token", "address",
      mojom::ExternalTransaction::New("transaction_id", "contribution_id",
                                      "destination", "amount"))
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostCommitTransactionUphold,
  Values(
    PostCommitTransactionUpholdParamType{
      "HTTP_200_response_not_a_dict",
      net::HTTP_OK,
      R"(
        [
          "status": "completed"
        ]
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionUpholdParamType{
      "HTTP_200_status_wrong_case",
      net::HTTP_OK,
      R"(
        {
          "STATUS": "completed"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionUpholdParamType{
      "HTTP_200_transaction_pending",
      net::HTTP_OK,
      R"(
        {
          "status": "processing"
        }
      )",
      base::unexpected(Error::kTransactionPending)
    },
    PostCommitTransactionUpholdParamType{
      "HTTP_200_unexpected_transaction_status",
      net::HTTP_OK,
      R"(
        {
          "status": "failed"
        }
      )",
      base::unexpected(Error::kUnexpectedTransactionStatus)
    },
    PostCommitTransactionUpholdParamType{
      "HTTP_200_transaction_completed",
      net::HTTP_OK,
      R"(
        {
          "status": "completed"
        }
      )",
      {}
    },
    PostCommitTransactionUpholdParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCommitTransactionUpholdParamType{
      "HTTP_404_transaction_not_found",
      net::HTTP_NOT_FOUND,
      "",
      base::unexpected(Error::kTransactionNotFound)
    },
    PostCommitTransactionUpholdParamType{
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
