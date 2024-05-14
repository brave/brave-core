/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_votes/post_votes.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsPostVotesTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().rewards_payment_url().Resolve(
            "/v1/votes");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::payment::PostVotes endpoint(engine());

    return WaitFor<mojom::Result>([&](auto callback) {
      mojom::UnblindedToken token;
      token.token_value =
          "s1OrSZUvo/33u3Y866mQaG/b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
          "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW"
          "9R6";
      token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

      credential::CredentialsRedeem redeem;
      redeem.publisher_key = "brave.com";
      redeem.type = mojom::RewardsType::ONE_TIME_TIP;
      redeem.processor = mojom::ContributionProcessor::UPHOLD;
      redeem.token_list = {std::move(token)};
      redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
      redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

      endpoint.Request(std::move(redeem), std::move(callback));
    });
  }
};

TEST_F(RewardsPostVotesTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
}

TEST_F(RewardsPostVotesTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 400;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
}

TEST_F(RewardsPostVotesTest, ServerError500) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
}

TEST_F(RewardsPostVotesTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;
  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal
