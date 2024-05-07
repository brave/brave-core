/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal::endpoints {

using Error = GetParameters::Error;
using Result = GetParameters::Result;

using GetParametersParamType =
    std::tuple<std::string,          // test name suffix
               net::HttpStatusCode,  // post create endpoint response status
               std::string,  // post create wallet endpoint response body
               base::RepeatingCallback<Result()>  // expected result
               >;

class RewardsGetParametersTest
    : public RewardsEngineTest,
      public WithParamInterface<GetParametersParamType> {};

TEST_P(RewardsGetParametersTest, Paths) {
  const auto& [ignore, status_code, body, make_result] = GetParam();

  Result expected_result = make_result.Run();

  auto request_url =
      engine().Get<EnvironmentConfig>().rewards_api_url().Resolve(
          "/v1/parameters");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(request_url.spec(), mojom::UrlMethod::GET,
                                      std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    RequestFor<endpoints::GetParameters>(engine()).Send(std::move(callback));
  });

  if (result.has_value()) {
    EXPECT_TRUE(expected_result.has_value());
    EXPECT_EQ(*result.value(), *expected_result.value());
  } else {
    EXPECT_FALSE(expected_result.has_value());
    EXPECT_EQ(result.error(), expected_result.error());
  }
}

INSTANTIATE_TEST_SUITE_P(
    RewardsGetParametersTest,
    RewardsGetParametersTest,
    Values(
        GetParametersParamType{
            "0_HTTP_200_success", net::HTTP_OK,
            R"(
              {
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

}  // namespace brave_rewards::internal::endpoints
