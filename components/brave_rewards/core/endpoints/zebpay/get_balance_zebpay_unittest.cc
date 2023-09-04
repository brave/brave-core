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
#include "brave/components/brave_rewards/core/endpoints/zebpay/get_balance_zebpay.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetBalanceZebPay*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = GetBalanceZebPay::Error;
using Result = GetBalanceZebPay::Result;

// clang-format off
using GetBalanceZebPayParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // GET balance ZebPay endpoint response status code
    std::string,          // GET balance ZebPay endpoint response body
    Result                // expected result
>;
// clang-format on

class GetBalanceZebPay : public TestWithParam<GetBalanceZebPayParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(GetBalanceZebPay, Paths) {
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

  RequestFor<endpoints::GetBalanceZebPay>(mock_engine_impl_, "token")
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  GetBalanceZebPay,
  Values(
    GetBalanceZebPayParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      R"(
        {
          "BAT": 0.0
        }
      )",
      0.0
    },
    GetBalanceZebPayParamType{
      "HTTP_200_response_is_not_a_dict",
      net::HTTP_OK,
      "[]",
      base::unexpected(Error::kFailedToParseBody)
    },
    GetBalanceZebPayParamType{
      "HTTP_200_no_BAT",
      net::HTTP_OK,
      "{}",
      base::unexpected(Error::kFailedToParseBody)
    },
    GetBalanceZebPayParamType{
      "HTTP_200_BAT_is_not_of_the_right_type",
      net::HTTP_OK,
      R"(
        {
          "BAT": "0.0"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    GetBalanceZebPayParamType{
      "HTTP_401",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    GetBalanceZebPayParamType{
      "HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
