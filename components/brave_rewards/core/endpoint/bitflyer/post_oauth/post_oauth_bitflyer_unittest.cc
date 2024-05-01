/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsBitflyerPostOauthTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url = engine().Get<EnvironmentConfig>().bitflyer_url().Resolve(
        "/api/link/v1/token");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::bitflyer::PostOauth endpoint(engine());

    return WaitForValues<mojom::Result, std::string&&, std::string&&,
                         std::string&&>([&](auto callback) {
      endpoint.Request(
          "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
          "4c2b665ca060d912fec5c735c734859a06118cc8", "1234567890",
          std::move(callback));
    });
  }
};

TEST_F(RewardsBitflyerPostOauthTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "access_token": "mock_access_token",
        "refresh_token": "mock_refresh_token",
        "expires_in": 259002,
        "scope": "assets create_deposit_id withdraw_to_deposit_id",
        "account_hash": "ad0fd9160be16790893ff021b2f9ccf7f14b5a9f",
        "token_type": "Bearer",
        "linking_info": "mock_linking_info",
        "deposit_id": "339dc5ff-1167-4d69-8dd8-aa77ccb12d74"
      })";

  auto [result, access_token, address, linking_info] =
      Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(access_token, "mock_access_token");
  EXPECT_EQ(address, "339dc5ff-1167-4d69-8dd8-aa77ccb12d74");
  EXPECT_EQ(linking_info, "mock_linking_info");
}

TEST_F(RewardsBitflyerPostOauthTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, access_token, address, linking_info] =
      Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(access_token, "");
  EXPECT_EQ(address, "");
  EXPECT_EQ(linking_info, "");
}

}  // namespace brave_rewards::internal
