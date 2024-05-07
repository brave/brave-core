/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_uphold/post_transaction_uphold.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsPostTransactionUpholdTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().rewards_payment_url().Resolve(
            "/v1/orders/f2e6494e-fb21-44d1-90e9-b5408799acd8"
            "/transactions/uphold");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::payment::PostTransactionUphold endpoint(engine());

    return WaitFor<mojom::Result>([&](auto callback) {
      mojom::SKUTransaction transaction;
      transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
      transaction.external_transaction_id =
          "d382d3ae-8462-4b2c-9b60-b669539f41b2";

      endpoint.Request(std::move(transaction), std::move(callback));
    });
  }
};

TEST_F(RewardsPostTransactionUpholdTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 201;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
}

TEST_F(RewardsPostTransactionUpholdTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 400;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostTransactionUpholdTest, ServerError404) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 404;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::NOT_FOUND);
}

TEST_F(RewardsPostTransactionUpholdTest, ServerError409) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 409;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostTransactionUpholdTest, ServerError500) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostTransactionUpholdTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal
