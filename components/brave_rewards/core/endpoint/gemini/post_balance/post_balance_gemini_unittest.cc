/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/post_balance/post_balance_gemini.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostBalanceTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace gemini {

class GeminiPostBalanceTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostBalance balance_{mock_engine_impl_};
};

TEST_F(GeminiPostBalanceTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->url = request->url;
        response->body = R"([
              {
                  "type": "exchange",
                  "currency": "BTC",
                  "amount": "1000.01195318",
                  "available": "1000.01195318",
                  "availableForWithdrawal": "1000.01195318"
              },
              {
                  "type": "exchange",
                  "currency": "ETH",
                  "amount": "20000",
                  "available": "20000",
                  "availableForWithdrawal": "20000"
              },
              {
                  "type": "exchange",
                  "currency": "BAT",
                  "amount": "5000",
                  "available": "5000",
                  "availableForWithdrawal": "5000"
              },
              {
                  "type": "exchange",
                  "currency": "USD",
                  "amount": "93687.50",
                  "available": "93677.40",
                  "availableForWithdrawal": "93677.40"
              }
            ])";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK, 5000.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostBalanceTest, ServerError401) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_UNAUTHORIZED;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN, 0.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostBalanceTest, ServerError403) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_FORBIDDEN;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN, 0.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GeminiPostBalanceTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 418;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, 0.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::internal
