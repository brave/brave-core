/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/parameters/rewards_parameters_provider.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

namespace {

constexpr char kCachedParametersJSON[] = R"(
    {
      "payout_status": {
        "bitflyer": "processing",
        "gemini": "processing",
        "solana": "processing",
        "unverified": "processing",
        "uphold": "processing",
        "zebpay": ""
      },
      "rate": 0.25,
      "tip": {
        "choices": [1.25, 5.0, 10.5],
        "monthly_choices": [1.25, 5.0, 10.5]
      },
      "tos_version": 1,
      "vbat_deadline": "13343184000000000",
      "vbat_expired": true,
      "wallet_provider_regions": {
        "bitflyer": {
          "allow": ["JP"],
          "block": []
        },
        "gemini": {
          "allow": ["US", "SG", "GB", "CA"],
          "block": []
        },
        "solana": {
          "allow": [],
          "block": ["KP", "ES"]
        },
        "uphold": {
          "allow": ["GB", "US"],
          "block": []
        },
        "zebpay": {
          "allow": ["IN"],
          "block": []
        }
      }
    })";

constexpr char kParametersEndpointResponse[] = R"(
    {
      "batRate": 0.3,
      "custodianRegions": {
        "bitflyer": {
          "allow": [],
          "block": []
        },
        "gemini": {
          "allow": [],
          "block": []
        },
        "uphold": {
          "allow": [],
          "block": []
        }
      },
      "payoutStatus": {},
      "tips": {
        "defaultMonthlyChoices": ["0"],
        "defaultTipChoices": ["0"]
      },
      "vbatDeadline": "2022-12-24T15:04:45.352584Z",
      "vbatExpired": true,
      "tosVersion": 3
    })";

}  // namespace

class RewardsParametersProviderTest : public RewardsEngineTest {
 protected:
  void AddParametersResponse(mojom::UrlResponsePtr response) {
    std::string url = engine()
                          .Get<EnvironmentConfig>()
                          .rewards_api_url()
                          .Resolve("/v1/parameters")
                          .spec();

    client().AddNetworkResultForTesting(url, mojom::UrlMethod::GET,
                                        std::move(response));
  }
};

TEST_F(RewardsParametersProviderTest, DictToParameters) {
  auto value = base::JSONReader::Read("{}");
  EXPECT_FALSE(RewardsParametersProvider::DictToParameters(value->GetDict()));

  value = base::JSONReader::Read(kCachedParametersJSON);
  auto params = RewardsParametersProvider::DictToParameters(value->GetDict());
  ASSERT_TRUE(params);

  EXPECT_EQ(params->payout_status.at("uphold"), "processing");
  EXPECT_EQ(params->rate, 0.25);
  EXPECT_EQ(params->tip_choices, (std::vector<double>{1.25, 5, 10.5}));
  EXPECT_EQ(params->monthly_tip_choices, (std::vector<double>{1.25, 5, 10.5}));
  EXPECT_EQ(params->tos_version, 1);
  base::Time vbat_deadline;
  EXPECT_TRUE(base::Time::FromUTCString("2023-10-31 00:00:00", &vbat_deadline));
  EXPECT_EQ(params->vbat_deadline, vbat_deadline);
  EXPECT_EQ(params->vbat_expired, true);
  EXPECT_EQ(params->wallet_provider_regions.at("uphold")->allow,
            (std::vector<std::string>({"GB", "US"})));
}

TEST_F(RewardsParametersProviderTest, GetParametersCached) {
  engine().SetState(state::kParameters,
                    *base::JSONReader::Read(kCachedParametersJSON));

  auto params = WaitFor<mojom::RewardsParametersPtr>([&](auto callback) {
    engine().Get<RewardsParametersProvider>().GetParameters(
        std::move(callback));
  });

  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.25);
}

TEST_F(RewardsParametersProviderTest, GetParameters) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body = kParametersEndpointResponse;
  AddParametersResponse(std::move(response));

  auto& provider = engine().Get<RewardsParametersProvider>();

  auto params = WaitFor<mojom::RewardsParametersPtr>(
      [&](auto callback) { provider.GetParameters(std::move(callback)); });

  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.3);

  auto cached_params = provider.GetCachedParameters();
  ASSERT_TRUE(cached_params);
  EXPECT_EQ(cached_params->rate, 0.3);
}

TEST_F(RewardsParametersProviderTest, EndpointError) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR;
  AddParametersResponse(std::move(response));

  engine().SetState(state::kParameters,
                    *base::JSONReader::Read(kCachedParametersJSON));

  auto& provider = engine().Get<RewardsParametersProvider>();

  auto params = WaitFor<mojom::RewardsParametersPtr>(
      [&](auto callback) { provider.GetParameters(std::move(callback)); });

  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.25);
}

TEST_F(RewardsParametersProviderTest, GetCachedParameters) {
  auto& provider = engine().Get<RewardsParametersProvider>();
  EXPECT_FALSE(provider.GetCachedParameters());

  engine().SetState(state::kParameters,
                    *base::JSONReader::Read(kCachedParametersJSON));

  auto params = provider.GetCachedParameters();
  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.25);
}

}  // namespace brave_rewards::internal
