/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_oauth/post_oauth_gemini.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGeminiPostOauthTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().gemini_oauth_url().Resolve(
            "/auth/token");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::gemini::PostOauth endpoint(engine());

    return WaitForValues<mojom::Result, std::string&&>([&](auto callback) {
      endpoint.Request(
          "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
          "1234567890", std::move(callback));
    });
  }
};

TEST_F(RewardsGeminiPostOauthTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "access_token": "aaaaa",
        "expires_in": 83370,
        "scope": "sample:scope",
        "refresh_token":"bbbbb",
        "token_type": "Bearer"
      })";

  auto [result, token] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(token, "aaaaa");
}

TEST_F(RewardsGeminiPostOauthTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 418;

  auto [result, token] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(token, "");
}

}  // namespace brave_rewards::internal
