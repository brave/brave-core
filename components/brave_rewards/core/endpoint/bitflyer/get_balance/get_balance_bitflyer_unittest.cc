/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/bitflyer/get_balance/get_balance_bitflyer.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetBalanceTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace bitflyer {

class GetBalanceTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  GetBalance balance_{mock_engine_impl_};
};

TEST_F(GetBalanceTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
              "account_hash": "ad0fd9160be16790893ff021b2f9ccf7f14b5a9f",
              "inventory": [
                {
                  "currency_code": "JPY",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "BTC",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "BCH",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "ETH",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "ETC",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "LTC",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "MONA",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "LSK",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "XRP",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "BAT",
                  "amount": 4.0,
                  "available": 4.0
                },
                {
                  "currency_code": "XLM",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "XEM",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "XTZ",
                  "amount": 0.0,
                  "available": 0.0
                }
              ]
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK, 4.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetBalanceTest, ServerError401) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 401;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN, 0.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetBalanceTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetBalanceCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, 0.0)).Times(1);
  balance_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace brave_rewards::internal
