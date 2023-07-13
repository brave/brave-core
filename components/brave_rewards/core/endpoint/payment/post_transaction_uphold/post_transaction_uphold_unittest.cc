/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_uphold/post_transaction_uphold.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostTransactionUpholdTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

class PostTransactionUpholdTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostTransactionUphold order_{mock_engine_impl_};
};

TEST_F(PostTransactionUpholdTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 201;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  MockFunction<PostTransactionUpholdCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::OK)).Times(1);
  order_.Request(transaction, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostTransactionUpholdTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  MockFunction<PostTransactionUpholdCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  order_.Request(transaction, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostTransactionUpholdTest, ServerError404) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 404;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  MockFunction<PostTransactionUpholdCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::NOT_FOUND)).Times(1);
  order_.Request(transaction, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostTransactionUpholdTest, ServerError409) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 409;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  MockFunction<PostTransactionUpholdCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  order_.Request(transaction, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostTransactionUpholdTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  MockFunction<PostTransactionUpholdCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  order_.Request(transaction, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostTransactionUpholdTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  MockFunction<PostTransactionUpholdCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  order_.Request(transaction, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
