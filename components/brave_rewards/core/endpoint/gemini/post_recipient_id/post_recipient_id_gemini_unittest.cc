/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostRecipientIdTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace gemini {

class GeminiPostRecipientIdTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostRecipientId post_recipient_id_{mock_engine_impl_};
};

TEST_F(GeminiPostRecipientIdTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->url = request->url;
        response->body = R"({
              "result": "OK",
              "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
              "label": "deposit_address"
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostRecipientIdCallback> callback;
  EXPECT_CALL(callback,
              Run(mojom::Result::OK,
                  std::string("60f9be89-ada7-486d-9cef-f6d3a10886d7")))
      .Times(1);
  post_recipient_id_.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                             callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostRecipientIdTest, ServerOK_Unverified) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->url = request->url;
        response->body = R"({
              "result": "OK",
              "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
              "label": "deposit_address"
            })";
        response->headers.insert(std::pair<std::string, std::string>(
            "www-authenticate", "Bearer error=\"unverified_account\""));
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostRecipientIdCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::NOT_FOUND, std::string())).Times(1);
  post_recipient_id_.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                             callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostRecipientIdTest, ServerError401) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_UNAUTHORIZED;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostRecipientIdCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN, std::string()))
      .Times(1);
  post_recipient_id_.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                             callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostRecipientIdTest, ServerError403) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_FORBIDDEN;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostRecipientIdCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN, std::string()))
      .Times(1);
  post_recipient_id_.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                             callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostRecipientIdTest, ServerError404) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_NOT_FOUND;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostRecipientIdCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::NOT_FOUND, std::string())).Times(1);
  post_recipient_id_.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                             callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostRecipientIdTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 418;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostRecipientIdCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, std::string())).Times(1);
  post_recipient_id_.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                             callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::internal
