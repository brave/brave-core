/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/post_oauth/post_oauth_gemini.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostOauthTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace gemini {

class GeminiPostOauthTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostOauth oauth_{mock_engine_impl_};
};

TEST_F(GeminiPostOauthTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->url = request->url;
        response->body = R"({
             "access_token": "aaaaa",
             "expires_in": 83370,
             "scope": "sample:scope",
             "refresh_token":"bbbbb",
             "token_type": "Bearer"
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostOauthCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK, std::string("aaaaa"))).Times(1);
  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostOauthTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 418;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostOauthCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, std::string())).Times(1);
  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::internal
