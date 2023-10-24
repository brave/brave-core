/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/put_safetynet/put_safetynet.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PutSafetynetTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class PutSafetynetTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PutSafetynet safetynet_{mock_engine_impl_};
};

TEST_F(PutSafetynetTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PutSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK)).Times(1);
  safetynet_.Request("sdfsdf32d323d23d", "dfasdfasdpflsadfplf2r23re2",
                     callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PutSafetynetTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PutSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::CAPTCHA_FAILED)).Times(1);
  safetynet_.Request("sdfsdf32d323d23d", "dfasdfasdpflsadfplf2r23re2",
                     callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PutSafetynetTest, ServerError401) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 401;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PutSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::CAPTCHA_FAILED)).Times(1);
  safetynet_.Request("sdfsdf32d323d23d", "dfasdfasdpflsadfplf2r23re2",
                     callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PutSafetynetTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PutSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED)).Times(1);
  safetynet_.Request("sdfsdf32d323d23d", "dfasdfasdpflsadfplf2r23re2",
                     callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
