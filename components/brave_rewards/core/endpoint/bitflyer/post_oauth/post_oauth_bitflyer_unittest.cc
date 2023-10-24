/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BitflyerPostOauthTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace bitflyer {

class BitflyerPostOauthTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostOauth oauth_{mock_engine_impl_};
};

TEST_F(BitflyerPostOauthTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostOauthCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK, std::string("mock_access_token"),
                            std::string("339dc5ff-1167-4d69-8dd8-aa77ccb12d74"),
                            std::string("mock_linking_info")))
      .Times(1);
  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "4c2b665ca060d912fec5c735c734859a06118cc8", "1234567890", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(BitflyerPostOauthTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostOauthCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, std::string(), _, _))
      .Times(1);
  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "4c2b665ca060d912fec5c735c734859a06118cc8", "1234567890", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace brave_rewards::internal
