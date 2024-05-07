/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_account/post_account_gemini.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGeminiPostAccountTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().gemini_api_url().Resolve(
            "/v1/account");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::gemini::PostAccount endpoint(engine());

    return WaitForValues<mojom::Result, std::string&&, std::string&&,
                         std::string&&>([&](auto callback) {
      endpoint.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsGeminiPostAccountTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "account": {
          "accountName": "Primary",
          "shortName": "primary",
          "type": "exchange",
          "created": "1619040615242",
          "verificationToken": "mocktoken"
        },
        "users": [{
          "name": "Test",
          "lastSignIn": "2021-04-30T18:46:03.017Z",
          "status": "Active",
          "countryCode": "US",
          "isVerified": true
        }],
        "memo_reference_code": "GEMAPLLV"
      })";

  auto [result, linking_info, user_name, country_id] =
      Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(linking_info, "mocktoken");
  EXPECT_EQ(user_name, "Test");
  EXPECT_EQ(country_id, "US");
}

TEST_F(RewardsGeminiPostAccountTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto [result, linking_info, user_name, country_id] =
      Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(linking_info, "");
  EXPECT_EQ(user_name, "");
  EXPECT_EQ(country_id, "");
}

TEST_F(RewardsGeminiPostAccountTest, ServerError403) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 403;

  auto [result, linking_info, user_name, country_id] =
      Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(linking_info, "");
  EXPECT_EQ(user_name, "");
  EXPECT_EQ(country_id, "");
}

TEST_F(RewardsGeminiPostAccountTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 418;

  auto [result, linking_info, user_name, country_id] =
      Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(linking_info, "");
  EXPECT_EQ(user_name, "");
  EXPECT_EQ(country_id, "");
}

}  // namespace brave_rewards::internal
