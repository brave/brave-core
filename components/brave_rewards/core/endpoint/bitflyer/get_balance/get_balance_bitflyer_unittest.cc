/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/get_balance/get_balance_bitflyer.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGetBalanceBitflyerTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url = engine().Get<EnvironmentConfig>().bitflyer_url().Resolve(
        "/api/link/v1/account/inventory");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::GET, std::move(response));

    endpoint::bitflyer::GetBalance endpoint(engine());

    return WaitForValues<mojom::Result, double>([&](auto callback) {
      endpoint.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsGetBalanceBitflyerTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "account_hash": "ad0fd9160be16790893ff021b2f9ccf7f14b5a9f",
        "inventory": [
          {
            "currency_code": "JPY",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "BTC",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "BCH",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "ETH",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "ETC",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "LTC",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "MONA",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "LSK",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "XRP",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "BAT",
            "amount": 4.0,
            "available": 4.0
          },
          {
            "currency_code": "XLM",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "XEM",
            "amount": 0.0,
            "available": 0.0
          },
          {
            "currency_code": "XTZ",
            "amount": 0.0,
            "available": 0.0
          }
        ]
      })";

  auto [result, balance] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(balance, 4.0);
}

TEST_F(RewardsGetBalanceBitflyerTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto [result, balance] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(balance, 0.0);
}

TEST_F(RewardsGetBalanceBitflyerTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, balance] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(balance, 0.0);
}

}  // namespace brave_rewards::internal
