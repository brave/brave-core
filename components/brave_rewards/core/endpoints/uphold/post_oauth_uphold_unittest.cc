/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/endpoints/uphold/post_oauth_uphold.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostOAuthUphold*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostOAuthUphold::Error;
using Result = PostOAuthUphold::Result;

// clang-format off
using PostOAuthUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post OAuth Uphold endpoint response status code
    std::string,          // post OAuth Uphold endpoint response body
    Result                // expected result
>;
// clang-format on

class PostOAuthUphold : public TestWithParam<PostOAuthUpholdParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostOAuthUphold, Paths) {
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

  RequestFor<endpoints::PostOAuthUphold>(
      mock_engine_impl_, "bb50f9d4782fb86a4302ef18179033abb17c257f")
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostOAuthUphold,
  Values(
    PostOAuthUpholdParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      R"(
        {
          "access_token": "9fd84e43c803622cc65a63c6d380a47612d7d718",
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
      "9fd84e43c803622cc65a63c6d380a47612d7d718"
    },
    PostOAuthUpholdParamType{
      "HTTP_200_failed_to_parse_body",
      net::HTTP_OK,
      R"(
        {
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthUpholdParamType{
      "HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const TestParamInfo<PostOAuthUpholdParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
