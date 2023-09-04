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
#include "brave/components/brave_rewards/core/endpoints/uphold/post_create_transaction_uphold.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostCreateTransactionUphold*

using ::testing::_;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostCreateTransactionUphold::Error;
using Result = PostCreateTransactionUphold::Result;

// clang-format off
using PostCreateTransactionUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create transaction endpoint status code
    std::string,          // post create transaction endpoint response body
    Result                // expected result
>;
// clang-format on

class PostCreateTransactionUphold
    : public TestWithParam<PostCreateTransactionUpholdParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostCreateTransactionUphold, Paths) {
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

  RequestFor<endpoints::PostCreateTransactionUphold>(
      mock_engine_impl_, "token", "address",
      mojom::ExternalTransaction::New("", "contribution_id", "destination",
                                      "amount"))
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostCreateTransactionUphold,
  Values(
    PostCreateTransactionUpholdParamType{
      "HTTP_202_response_not_a_dict",
      net::HTTP_ACCEPTED,
      R"(
        [
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        ]
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_202_id_wrong_case",
      net::HTTP_ACCEPTED,
      R"(
        {
          "ID": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_202_ok",
      net::HTTP_ACCEPTED,
      R"(
        {
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
      "87725361-4245-4435-a75a-f7a85674714a"
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCreateTransactionUpholdParamType{
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
