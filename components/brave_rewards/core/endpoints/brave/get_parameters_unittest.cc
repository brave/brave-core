/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal::endpoints {

class RewardsGetParametersTest : public RewardsEngineTest {
 protected:
  GetParameters::Result SendRequest(mojom::UrlResponsePtr response) {
    auto url = engine()
                   .Get<EnvironmentConfig>()
                   .rewards_api_url()
                   .Resolve("/v1/parameters")
                   .spec();

    AddNetworkResultForTesting(url, mojom::UrlMethod::GET, std::move(response));

    GetParameters endpoint(engine());
    auto [result] = WaitFor<GetParameters::Result>(
        [&](auto callback) { endpoint.Request(std::move(callback)); });

    return std::move(result);
  }
};

TEST_F(RewardsGetParametersTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
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
      })";

  auto result = SendRequest(std::move(response));
  EXPECT_TRUE(result.has_value());

  auto params = mojom::RewardsParameters::New();
  params->rate = 0.301298;
  params->auto_contribute_choice = 1;
  params->auto_contribute_choices = {1.0, 2.0, 3.0, 5.0, 7.0, 10.0, 20.0};
  params->tip_choices = {1.25, 5.0, 10.5};
  params->monthly_tip_choices = {1.25, 5.0, 10.5};
  params->payout_status = {{"bitflyer", "off"},
                           {"gemini", "off"},
                           {"unverified", "off"},
                           {"uphold", "complete"}};

  auto make_region_info = [](std::vector<std::string> allow,
                             std::vector<std::string> block) {
    return mojom::Regions::New(std::move(allow), std::move(block));
  };

  params->wallet_provider_regions.emplace("bitflyer",
                                          make_region_info({"JP"}, {}));
  params->wallet_provider_regions.emplace(
      "gemini",
      make_region_info({"AU", "AT", "BE", "CA", "CO", "DK", "FI", "HK", "IE",
                        "IT", "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"},
                       {}));
  params->wallet_provider_regions.emplace(
      "uphold",
      make_region_info({"AU", "AT", "BE", "CO", "DK", "FI", "HK", "IE", "IT",
                        "NL", "NO", "PT", "SG", "ES", "SE", "GB", "US"},
                       {}));

  ASSERT_TRUE(base::Time::FromUTCString("2022-12-24T15:04:45.352584Z",
                                        &params->vbat_deadline));

  params->vbat_expired = true;
  params->tos_version = 3;

  EXPECT_EQ(*result.value(), *params);
}

TEST_F(RewardsGetParametersTest, ServerError) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;

  auto result = SendRequest(std::move(response));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GetParameters::Error::kUnexpectedStatusCode);
}

TEST_F(RewardsGetParametersTest, InvalidBody) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = "{}";

  auto result = SendRequest(std::move(response));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), GetParameters::Error::kFailedToParseBody);
}

}  // namespace brave_rewards::internal::endpoints
