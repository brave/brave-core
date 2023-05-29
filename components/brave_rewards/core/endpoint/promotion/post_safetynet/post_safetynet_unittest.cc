/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_safetynet/post_safetynet.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostSafetynetTest.*

using ::testing::_;

namespace brave_rewards::internal::endpoint::promotion {

class PostSafetynetTest : public MockLedgerTest {
 protected:
  PostSafetynet safetynet_;
};

TEST_F(PostSafetynetTest, ServerOK) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
              "nonce": "c4645786-052f-402f-8593-56af2f7a21ce"
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::LEDGER_OK,
                            "c4645786-052f-402f-8593-56af2f7a21ce"))
      .Times(1);
  safetynet_.Request(callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostSafetynetTest, ServerError400) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::LEDGER_ERROR, "")).Times(1);
  safetynet_.Request(callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostSafetynetTest, ServerError401) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 401;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSafetynetCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::LEDGER_ERROR, "")).Times(1);
  safetynet_.Request(callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace brave_rewards::internal::endpoint::promotion
