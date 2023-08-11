/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetParameters*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = GetParameters::Error;
using Result = GetParameters::Result;

// clang-format off
using GetParametersParamType = std::tuple<
    std::string,             // test name suffix
    net::HttpStatusCode,     // post create wallet endpoint response status code
    std::string,             // post create wallet endpoint response body
    std::shared_ptr<Result>  // expected result
>;
// clang-format on

class GetParameters : public TestWithParam<GetParametersParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(GetParameters, Paths) {
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
  EXPECT_CALL(callback, Run).Times(1).WillOnce([&](Result&& result) {
    if (result.has_value()) {
      EXPECT_TRUE(expected_result->has_value());
      EXPECT_EQ(*result.value(), *expected_result->value());
    } else {
      EXPECT_FALSE(expected_result->has_value());
      EXPECT_EQ(result.error(), expected_result->error());
    }
  });

  RequestFor<endpoints::GetParameters>(mock_engine_impl_).Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  GetParameters,
  Values(
    GetParametersParamType{
      "0_HTTP_200_success",
      net::HTTP_OK,
      R"(
        {
          "autocontribute": {
            "choices": [1, 2.0, 3, "4.0", 5, "6.0", 7, "8.0", "9", 10.0, 20],
            "defaultChoice": 1
          },
          "batRate": 0.301298,
          "custodianRegions": {
            "bitflyer": {
              "allow": [ 1, 2.0, "JP"],
              "block": []
            },
            "gemini": {
              "allow": [ 1, 2.0, "AU", "AT", "BE", "CA", "CO", "DK", "FI", "HK", "IE", "IT", "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"],
              "block": []
            },
            "uphold": {
              "allow": [ 1, 2.0, "AU", "AT", "BE", "CO", "DK", "FI", "HK", "IE", "IT", "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"],
              "block": []
            }
          },
          "payoutStatus": {
            "bitflyer": "off",
            "gemini": "off",
            "unverified": "off",
            "uphold": "complete"
          },
          "tips": {
            "defaultMonthlyChoices": ["0", 1.25, 5, 10.5, "15"],
            "defaultTipChoices": ["0", 1.25, 5, 10.5, "15"]
          },
          "vbatDeadline": "2022-12-24T15:04:45.352584Z",
          "vbatExpired": true
        }
      )",
      std::make_shared<Result>(mojom::RewardsParameters::New(
          0.301298,                                          // rate
          1.0,                                               // auto_contribute_choice  // NOLINT
          std::vector{1.0, 2.0, 3.0, 5.0, 7.0, 10.0, 20.0},  // auto_contribute_choices // NOLINT
          std::vector{1.25, 5.0, 10.5},                      // tip_choices
          std::vector{1.25, 5.0, 10.5},                      // monthly_tip_choices     // NOLINT
          base::flat_map<std::string, std::string>{          // payout_status
              {"bitflyer", "off"},
              {"gemini", "off"},
              {"unverified", "off"},
              {"uphold", "complete"}},
          [] {                                               // wallet_provider_regions // NOLINT
            base::flat_map<std::string, mojom::RegionsPtr>
                wallet_provider_regions;
            wallet_provider_regions.emplace(
                "bitflyer",
                mojom::Regions::New(std::vector<std::string>{"JP"},
                                    std::vector<std::string>{}));
            wallet_provider_regions.emplace(
                "gemini", mojom::Regions::New(
                              std::vector<std::string>{
                                  "AU", "AT", "BE", "CA", "CO", "DK",
                                  "FI", "HK", "IE", "IT", "NL", "NO",
                                  "PT", "SG", "ES", "SE", "GB", "US"},
                              std::vector<std::string>{}));
            wallet_provider_regions.emplace(
                "uphold", mojom::Regions::New(
                              std::vector<std::string>{
                                  "AU", "AT", "BE", "CO", "DK", "FI",
                                  "HK", "IE", "IT", "NL", "NO", "PT",
                                  "SG", "ES", "SE", "GB", "US"},
                              std::vector<std::string>{}));

            return wallet_provider_regions;
          }(),
          [] {
            base::Time time;
            static_cast<void>(base::Time::FromUTCString("2022-12-24T15:04:45.352584Z", &time));
            return time;
          }(),
          true))
    },
    GetParametersParamType{
      "1_HTTP_500_failed_to_get_parameters",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      std::make_shared<Result>(base::unexpected(Error::kFailedToGetParameters))
    },
    GetParametersParamType{
      "2_HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      std::make_shared<Result>(base::unexpected(Error::kUnexpectedStatusCode))
    }),
  [](const TestParamInfo<GetParametersParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
