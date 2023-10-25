/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_captcha/post_captcha.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCaptchaTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class PostCaptchaTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostCaptcha captcha_{mock_engine_impl_};
};

TEST_F(PostCaptchaTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
              "hint": "circle",
              "captchaId": "d155d2d2-2627-425b-9be8-44ae9f541762"
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostCaptchaCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK, "circle",
                            "d155d2d2-2627-425b-9be8-44ae9f541762"))
      .Times(1);
  captcha_.Request(callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCaptchaTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostCaptchaCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _, _)).Times(1);
  captcha_.Request(callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostCaptchaTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostCaptchaCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _, _)).Times(1);
  captcha_.Request(callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
