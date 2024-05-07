/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/zebpay/get_balance_zebpay.h"

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

using Error = endpoints::GetBalanceZebPay::Error;
using Result = endpoints::GetBalanceZebPay::Result;

using GetBalanceZebPayParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // GET balance ZebPay endpoint response status code
    std::string,          // GET balance ZebPay endpoint response body
    Result                // expected result
>;

class RewardsGetBalanceZebPayTest
    : public RewardsEngineTest,
      public WithParamInterface<GetBalanceZebPayParamType> {};

TEST_P(RewardsGetBalanceZebPayTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().zebpay_api_url().Resolve(
      "/api/balance");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(request_url.spec(), mojom::UrlMethod::GET,
                                      std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::GetBalanceZebPay>(engine(), "token")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsGetBalanceZebPayTest,
    RewardsGetBalanceZebPayTest,
    Values(
        GetBalanceZebPayParamType{"HTTP_200_success", net::HTTP_OK,
                                  R"(
        {
          "BAT": 0.0
        }
      )",
                                  0.0},
        GetBalanceZebPayParamType{"HTTP_200_response_is_not_a_dict",
                                  net::HTTP_OK, "[]",
                                  base::unexpected(Error::kFailedToParseBody)},
        GetBalanceZebPayParamType{"HTTP_200_no_BAT", net::HTTP_OK, "{}",
                                  base::unexpected(Error::kFailedToParseBody)},
        GetBalanceZebPayParamType{"HTTP_200_BAT_is_not_of_the_right_type",
                                  net::HTTP_OK,
                                  R"(
        {
          "BAT": "0.0"
        }
      )",
                                  base::unexpected(Error::kFailedToParseBody)},
        GetBalanceZebPayParamType{"HTTP_401", net::HTTP_UNAUTHORIZED, "",
                                  base::unexpected(Error::kAccessTokenExpired)},
        GetBalanceZebPayParamType{
            "HTTP_503_unexpected_status_code", net::HTTP_SERVICE_UNAVAILABLE,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
