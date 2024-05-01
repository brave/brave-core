/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_credentials/post_credentials.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsPostCredentialsTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().rewards_payment_url().Resolve(
            "/v1/orders/pl2okf23-f2f02kf2fm2-msdkfsodkfds/credentials");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::payment::PostCredentials endpoint(engine());

    return WaitFor<mojom::Result>([&](auto callback) {
      base::Value::List blinded;
      blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

      endpoint.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                       "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                       std::move(blinded), std::move(callback));
    });
  }
};

TEST_F(RewardsPostCredentialsTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
}

TEST_F(RewardsPostCredentialsTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 400;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostCredentialsTest, ServerError409) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 409;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostCredentialsTest, ServerError500) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

TEST_F(RewardsPostCredentialsTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal
