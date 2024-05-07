/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGetPrefixListTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url = engine().Get<EnvironmentConfig>().rewards_url().Resolve(
        "/publishers/prefix-list");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::GET, std::move(response));

    endpoint::rewards::GetPrefixList endpoint(engine());

    return WaitForValues<mojom::Result, std::string>(
        [&](auto callback) { endpoint.Request(std::move(callback)); });
  }
};

TEST_F(RewardsGetPrefixListTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = "blob";

  auto [result, body] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(body, "blob");
}

TEST_F(RewardsGetPrefixListTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, body] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(body, "");
}

TEST_F(RewardsGetPrefixListTest, ServerBodyEmpty) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;

  auto [result, body] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(body, "");
}

}  // namespace brave_rewards::internal
