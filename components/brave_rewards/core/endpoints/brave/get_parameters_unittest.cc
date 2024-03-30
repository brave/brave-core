/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_mock.h"
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

using GetParametersParamType =
    std::tuple<std::string,          // test name suffix
               net::HttpStatusCode,  // post create endpoint response status
               std::string,  // post create wallet endpoint response body
               base::RepeatingCallback<Result()>  // expected result
               >;

class GetParameters : public TestWithParam<GetParametersParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngine mock_engine_;
};

TEST_P(GetParameters, Paths) {
  const auto& [ignore, status_code, body, make_result] = GetParam();

  Result expected_result = make_result.Run();

  EXPECT_CALL(*mock_engine_.mock_client(), LoadURL(_, _))
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
      EXPECT_TRUE(expected_result.has_value());
      EXPECT_EQ(*result.value(), *expected_result.value());
    } else {
      EXPECT_FALSE(expected_result.has_value());
      EXPECT_EQ(result.error(), expected_result.error());
    }
  });

  RequestFor<endpoints::GetParameters>(mock_engine_).Send(callback.Get());

  task_environment_.RunUntilIdle();
}

INSTANTIATE_TEST_SUITE_P(
    Endpoints,
    GetParameters,
    Values(
        GetParametersParamType{
            "0_HTTP_200_success", net::HTTP_OK,
            R"(
              {
                "autocontribute": {
                  "choices": [1, 2.0, 3, "4.0", 5, "6.0", 7, "8.0", "9",
                              10.0, 20],
                  "defaultChoice": 1
                },
                "batRate": 0.301298,
                "custodianRegions": {
                  "bitflyer": {
                    "allow": [ 1, 2.0, "JP"],
                    "block": []
                  },
                  "gemini": {
                    "allow": [ 1, 2.0, "AU", "AT", "BE", "CA", "CO", "DK", "FI",
                              "HK", "IE", "IT", "NL", "NO", "PT", "SG", "ES",
                              "SE", "GB", "US"],
                    "block": []
                  },
                  "uphold": {
                    "allow": [ 1, 2.0, "AU", "AT", "BE", "CO", "DK", "FI", "HK",
                              "IE", "IT", "NL", "NO", "PT", "SG", "ES", "SE",
                              "GB", "US"],
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
                "vbatExpired": true,
                "tosVersion": 3
              }
            )",
            base::BindRepeating([]() -> Result {
              auto params = mojom::RewardsParameters::New();
              params->rate = 0.301298;
              params->auto_contribute_choice = 1;
              params->auto_contribute_choices = {1.0, 2.0,  3.0, 5.0,
                                                 7.0, 10.0, 20.0};
              params->tip_choices = std::vector{1.25, 5.0, 10.5};
              params->monthly_tip_choices = std::vector{1.25, 5.0, 10.5};
              params->payout_status = {{"bitflyer", "off"},
                                       {"gemini", "off"},
                                       {"unverified", "off"},
                                       {"uphold", "complete"}};

              base::flat_map<std::string, mojom::RegionsPtr>
                  wallet_provider_regions;
              wallet_provider_regions.emplace(
                  "bitflyer",
                  mojom::Regions::New(std::vector<std::string>{"JP"},
                                      std::vector<std::string>{}));
              wallet_provider_regions.emplace(
                  "gemini",
                  mojom::Regions::New(
                      std::vector<std::string>{
                          "AU", "AT", "BE", "CA", "CO", "DK", "FI", "HK", "IE",
                          "IT", "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"},
                      std::vector<std::string>{}));
              wallet_provider_regions.emplace(
                  "uphold",
                  mojom::Regions::New(
                      std::vector<std::string>{
                          "AU", "AT", "BE", "CO", "DK", "FI", "HK", "IE", "IT",
                          "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"},
                      std::vector<std::string>{}));

              params->wallet_provider_regions =
                  std::move(wallet_provider_regions);

              CHECK(base::Time::FromUTCString("2022-12-24T15:04:45.352584Z",
                                              &params->vbat_deadline));

              params->vbat_expired = true;
              params->tos_version = 3;

              return params;
            })},
        GetParametersParamType{"1_HTTP_500_failed_to_get_parameters",
                               net::HTTP_INTERNAL_SERVER_ERROR, "",
                               base::BindRepeating([]() -> Result {
                                 return base::unexpected(
                                     Error::kFailedToGetParameters);
                               })},
        GetParametersParamType{
            "2_HTTP_503_unexpected_status_code", net::HTTP_SERVICE_UNAVAILABLE,
            "", base::BindRepeating([]() -> Result {
              return base::unexpected(Error::kUnexpectedStatusCode);
            })}),
    [](const TestParamInfo<GetParametersParamType>& info) {
      return std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal::endpoints::test
