/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_creds/post_creds.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCredsTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class PostCredsTest : public testing::Test {
 protected:
  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::string wallet = R"({
            "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
            "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
          })";
          std::move(callback).Run(std::move(wallet));
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostCreds creds_{mock_engine_impl_};
};

TEST_F(PostCredsTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
              "claimId": "53714048-9675-419e-baa3-369d85a2facb"
            })";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback,
              Run(mojom::Result::OK, "53714048-9675-419e-baa3-369d85a2facb"))
      .Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredsTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _)).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredsTest, ServerError403) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 403;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _)).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredsTest, ServerError409) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 409;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _)).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredsTest, ServerError410) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 410;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::NOT_FOUND, _)).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredsTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _)).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCredsTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  base::MockCallback<PostCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _)).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", std::move(creds),
                 callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
