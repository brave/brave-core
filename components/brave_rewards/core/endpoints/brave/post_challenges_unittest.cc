/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_challenges.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=RewardsPostChallengesTest*

namespace brave_rewards::internal::endpoints {

class RewardsPostChallengesTest : public RewardsEngineTest {
 protected:
  void SetUp() override {
    std::string json = R"({
          "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })";
    engine().SetState(state::kWalletBrave, std::move(json));
  }

  PostChallenges::Result SendRequest(mojom::UrlResponsePtr response) {
    AddNetworkResultForTesting(
        endpoint::promotion::GetServerUrl("/v3/wallet/challenges"),
        mojom::UrlMethod::POST, std::move(response));

    PostChallenges endpoint(engine());

    auto [result] = WaitFor<PostChallenges::Result>(
        [&](auto callback) { endpoint.Request(std::move(callback)); });

    return std::move(result);
  }
};

TEST_F(RewardsPostChallengesTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_BAD_REQUEST;
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result, base::unexpected(PostChallenges::Error::kInvalidRequest));
}

TEST_F(RewardsPostChallengesTest, ServerCreated) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_CREATED;
  response->body = R"({"challengeId": "368d87a3-7749-4ebb-9f3a-2882c99078c7"})";
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result.value(), "368d87a3-7749-4ebb-9f3a-2882c99078c7");
}

}  // namespace brave_rewards::internal::endpoints
