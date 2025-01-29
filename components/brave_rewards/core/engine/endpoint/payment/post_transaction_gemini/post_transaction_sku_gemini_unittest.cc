/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/engine/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"

namespace brave_rewards::internal {

class RewardsPostTransactionGeminiTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().rewards_payment_url().Resolve(
            "/v1/orders/f2e6494e-fb21-44d1-90e9-b5408799acd8"
            "/transactions/gemini");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::payment::PostTransactionGemini endpoint(engine());

    return WaitFor<mojom::Result>([&](auto callback) {
      mojom::SKUTransaction transaction;
      transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
      transaction.external_transaction_id =
          "d382d3ae-8462-4b2c-9b60-b669539f41b2";

      endpoint.Request(std::move(transaction), std::move(callback));
    });
  }
};

TEST_F(RewardsPostTransactionGeminiTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 201;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
}

TEST_F(RewardsPostTransactionGeminiTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 400;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostTransactionGeminiTest, ServerError404) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 404;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::NOT_FOUND);
}

TEST_F(RewardsPostTransactionGeminiTest, ServerError409) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 409;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostTransactionGeminiTest, ServerError500) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostTransactionGeminiTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 418;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal
