/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/parameters/rewards_parameters_provider.h"

#include <optional>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

namespace {

constexpr char kCachedParametersJSON[] = R"(
    {
      "ac": {
        "choice": 1.0,
        "choices": [1.0, 2.0, 3.0, 5.0, 7.0, 10.0, 20.0]
      },
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

}  // namespace

class RewardsParametersProviderTest : public RewardsEngineTest {
 protected:
  struct MockEndpoint : public endpoints::GetParameters {
    explicit MockEndpoint(RewardsEngineImpl& engine)
        : endpoints::GetParameters(engine) {}

    void Request(RequestCallback callback) override {
      std::move(callback).Run(std::move(*endpoint_result));
    }

    std::optional<Result> endpoint_result;
  };
};

TEST_F(RewardsParametersProviderTest, ValueToParameters) {
  EXPECT_FALSE(RewardsParametersProvider::ValueToParameters(
      *base::JSONReader::Read("{}")));

  auto params = RewardsParametersProvider::ValueToParameters(
      *base::JSONReader::Read(kCachedParametersJSON));

  ASSERT_TRUE(params);

  EXPECT_EQ(params->auto_contribute_choice, 1.0);
  EXPECT_EQ(params->auto_contribute_choices,
            (std::vector<double>{1, 2, 3, 5, 7, 10, 20}));
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

  auto [params] = WaitFor<mojom::RewardsParametersPtr>([&](auto callback) {
    engine().Get<RewardsParametersProvider>().GetParameters(
        std::move(callback));
  });

  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.25);
}

TEST_F(RewardsParametersProviderTest, GetParameters) {
  auto endpoint_params = mojom::RewardsParameters::New();
  endpoint_params->rate = .5;

  auto endpoint = std::make_unique<MockEndpoint>(engine());
  endpoint->endpoint_result = endpoint_params->Clone();

  auto& provider = engine().Get<RewardsParametersProvider>();
  provider.SetParametersEndpointForTesting(std::move(endpoint));

  auto [params] = WaitFor<mojom::RewardsParametersPtr>(
      [&](auto callback) { provider.GetParameters(std::move(callback)); });

  ASSERT_TRUE(params);
  EXPECT_EQ(*params, *endpoint_params);

  auto cached_params = provider.LoadParameters();
  ASSERT_TRUE(cached_params);
  EXPECT_EQ(*cached_params, *endpoint_params);
}

TEST_F(RewardsParametersProviderTest, EndpointError) {
  engine().SetState(state::kParameters,
                    *base::JSONReader::Read(kCachedParametersJSON));

  auto endpoint = std::make_unique<MockEndpoint>(engine());
  endpoint->endpoint_result =
      base::unexpected(endpoints::GetParameters::Error::kUnexpectedStatusCode);

  auto& provider = engine().Get<RewardsParametersProvider>();
  provider.SetParametersEndpointForTesting(std::move(endpoint));

  auto [params] = WaitFor<mojom::RewardsParametersPtr>(
      [&](auto callback) { provider.GetParameters(std::move(callback)); });

  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.25);
}

TEST_F(RewardsParametersProviderTest, LoadParameters) {
  auto& provider = engine().Get<RewardsParametersProvider>();
  EXPECT_FALSE(provider.LoadParameters());

  engine().SetState(state::kParameters,
                    *base::JSONReader::Read(kCachedParametersJSON));

  auto params = provider.LoadParameters();
  ASSERT_TRUE(params);
  EXPECT_EQ(params->rate, 0.25);
}

}  // namespace brave_rewards::internal
