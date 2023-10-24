/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_clobbered_claims/post_clobbered_claims.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostClobberedClaimsTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class PostClobberedClaimsTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostClobberedClaims claims_{mock_engine_impl_};
};

TEST_F(PostClobberedClaimsTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  MockFunction<PostClobberedClaimsCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::OK)).Times(1);
  claims_.Request(std::move(corrupted_claims), callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostClobberedClaimsTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  MockFunction<PostClobberedClaimsCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  claims_.Request(std::move(corrupted_claims), callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostClobberedClaimsTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  MockFunction<PostClobberedClaimsCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  claims_.Request(std::move(corrupted_claims), callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostClobberedClaimsTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  MockFunction<PostClobberedClaimsCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  claims_.Request(std::move(corrupted_claims), callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
