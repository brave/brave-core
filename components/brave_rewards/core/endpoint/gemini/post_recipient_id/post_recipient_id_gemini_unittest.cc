/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGeminiPostRecipientIdTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().gemini_api_url().Resolve(
            "/v1/payments/recipientIds");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::gemini::PostRecipientId endpoint(engine());

    return WaitForValues<mojom::Result, std::string&&>([&](auto callback) {
      endpoint.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsGeminiPostRecipientIdTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "result": "OK",
        "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
        "label": "deposit_address"
      })";

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(id, "60f9be89-ada7-486d-9cef-f6d3a10886d7");
}

TEST_F(RewardsGeminiPostRecipientIdTest, ServerOK_Unverified) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "result": "OK",
        "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
        "label": "deposit_address"
      })";
  response->headers.insert(
      {"www-authenticate", "Bearer error=\"unverified_account\""});

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::NOT_FOUND);
  EXPECT_EQ(id, "");
}

TEST_F(RewardsGeminiPostRecipientIdTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(id, "");
}

TEST_F(RewardsGeminiPostRecipientIdTest, ServerError403) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 403;

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(id, "");
}

TEST_F(RewardsGeminiPostRecipientIdTest, ServerError404) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 404;

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::NOT_FOUND);
  EXPECT_EQ(id, "");
}

TEST_F(RewardsGeminiPostRecipientIdTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 418;

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(id, "");
}

}  // namespace brave_rewards::internal
