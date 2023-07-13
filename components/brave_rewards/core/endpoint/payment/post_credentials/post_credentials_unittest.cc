/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_credentials/post_credentials.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCredentialsTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

class PostCredentialsTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostCredentials creds_{mock_engine_impl_};
};

TEST_F(PostCredentialsTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredentialsTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredentialsTest, ServerError409) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 409;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredentialsTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredentialsTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
