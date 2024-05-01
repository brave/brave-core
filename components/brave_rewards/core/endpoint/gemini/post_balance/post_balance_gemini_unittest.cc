/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_balance/post_balance_gemini.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGeminiPostBalanceTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().gemini_api_url().Resolve(
            "/v1/balances");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::gemini::PostBalance endpoint(engine());

    return WaitForValues<mojom::Result, double>([&](auto callback) {
      endpoint.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsGeminiPostBalanceTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"([
        {
            "type": "exchange",
            "currency": "BTC",
            "amount": "1000.01195318",
            "available": "1000.01195318",
            "availableForWithdrawal": "1000.01195318"
        },
        {
            "type": "exchange",
            "currency": "ETH",
            "amount": "20000",
            "available": "20000",
            "availableForWithdrawal": "20000"
        },
        {
            "type": "exchange",
            "currency": "BAT",
            "amount": "5000",
            "available": "5000",
            "availableForWithdrawal": "5000"
        },
        {
            "type": "exchange",
            "currency": "USD",
            "amount": "93687.50",
            "available": "93677.40",
            "availableForWithdrawal": "93677.40"
        }
      ])";

  auto [result, balance] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(balance, 5000.0);
}

TEST_F(RewardsGeminiPostBalanceTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto [result, balance] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(balance, 0.0);
}

TEST_F(RewardsGeminiPostBalanceTest, ServerError403) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 403;

  auto [result, balance] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(balance, 0.0);
}

TEST_F(RewardsGeminiPostBalanceTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 418;

  auto [result, balance] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(balance, 0.0);
}

}  // namespace brave_rewards::internal
