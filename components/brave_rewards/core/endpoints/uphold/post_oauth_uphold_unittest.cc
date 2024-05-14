/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_oauth_uphold.h"

#include <string>
#include <tuple>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal {
using Error = endpoints::PostOAuthUphold::Error;
using Result = endpoints::PostOAuthUphold::Result;

using PostOAuthUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post OAuth Uphold endpoint response status code
    std::string,          // post OAuth Uphold endpoint response body
    Result                // expected result
>;

class RewardsPostOAuthUpholdTest
    : public RewardsEngineTest,
      public WithParamInterface<PostOAuthUpholdParamType> {};

TEST_P(RewardsPostOAuthUpholdTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
      "/oauth2/token");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::PostOAuthUphold>(
        engine(), "bb50f9d4782fb86a4302ef18179033abb17c257f")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostOAuthUpholdTest,
    RewardsPostOAuthUpholdTest,
    Values(
        PostOAuthUpholdParamType{"HTTP_200_success", net::HTTP_OK,
                                 R"(
        {
          "access_token": "9fd84e43c803622cc65a63c6d380a47612d7d718",
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
                                 "9fd84e43c803622cc65a63c6d380a47612d7d718"},
        PostOAuthUpholdParamType{"HTTP_2xx_success", net::HTTP_PARTIAL_CONTENT,
                                 R"(
        {
          "access_token": "9fd84e43c803622cc65a63c6d380a47612d7d718",
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
                                 "9fd84e43c803622cc65a63c6d380a47612d7d718"},
        PostOAuthUpholdParamType{"HTTP_200_failed_to_parse_body", net::HTTP_OK,
                                 R"(
        {
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthUpholdParamType{
            "HTTP_503_unexpected_status_code", net::HTTP_SERVICE_UNAVAILABLE,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const TestParamInfo<PostOAuthUpholdParamType>& info) {
      return std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal
