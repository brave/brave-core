/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/zebpay/post_oauth_zebpay.h"

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

using Error = endpoints::PostOAuthZebPay::Error;
using Result = endpoints::PostOAuthZebPay::Result;

using PostOAuthZebPayParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post OAuth ZebPay endpoint response status code
    std::string,          // post OAuth ZebPay endpoint response body
    Result                // expected result
>;

class RewardsPostOAuthZebPayTest
    : public RewardsEngineTest,
      public WithParamInterface<PostOAuthZebPayParamType> {};

TEST_P(RewardsPostOAuthZebPayTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url =
      engine().Get<EnvironmentConfig>().zebpay_oauth_url().Resolve(
          "/connect/token");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::PostOAuthZebPay>(engine(), "code")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostOAuthZebPayTest,
    RewardsPostOAuthZebPayTest,
    Values(
        PostOAuthZebPayParamType{
            "HTTP_200_success", net::HTTP_OK,
            R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.eyJkZXBvc2l0SWQiOiIxMTExMSJ9.zzz"
        }
      )",
            std::tuple{"af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf09704"
                       "6ab72abbc",
                       "xxx.eyJkZXBvc2l0SWQiOiIxMTExMSJ9.zzz", "11111"}},
        PostOAuthZebPayParamType{"HTTP_200_body_is_not_a_dict", net::HTTP_OK,
                                 "[]",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthZebPayParamType{"HTTP_200_access_token_is_empty", net::HTTP_OK,
                                 R"(
        {
          "access_token": ""
        }
      )",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthZebPayParamType{"HTTP_200_linking_info_is_empty", net::HTTP_OK,
                                 R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": ""
        }
      )",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthZebPayParamType{"HTTP_200_linking_info_not_enough_components",
                                 net::HTTP_OK,
                                 R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.yyy"
        }
      )",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthZebPayParamType{"HTTP_200_linking_info_payload_is_not_a_dict",
                                 net::HTTP_OK,
                                 R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.W10=.zzz"
        }
      )",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthZebPayParamType{"HTTP_200_deposit_id_is_empty", net::HTTP_OK,
                                 R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.eyJkZXBvc2l0SWQiOiIifQ==.zzz"
        }
      )",
                                 base::unexpected(Error::kFailedToParseBody)},
        PostOAuthZebPayParamType{
            "HTTP_503_unexpected_status_code", net::HTTP_SERVICE_UNAVAILABLE,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
