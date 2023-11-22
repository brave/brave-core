/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_order/post_order.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostOrderTest.*

using ::testing::_;
using ::testing::IsFalse;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

class PostOrderTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostOrder order_{mock_engine_impl_};
};

TEST_F(PostOrderTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 201;
        response->url = request->url;
        response->body = R"({
              "id": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
              "createdAt": "2020-06-10T18:58:21.378752Z",
              "currency": "BAT",
              "updatedAt": "2020-06-10T18:58:21.378752Z",
              "totalPrice": "1",
              "merchantId": "",
              "location": "brave.com",
              "status": "pending",
              "items": [
               {
                 "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
                 "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
                 "sku": "user-wallet-vote",
                 "createdAt": "2020-06-10T18:58:21.378752Z",
                 "updatedAt": "2020-06-10T18:58:21.378752Z",
                 "currency": "BAT",
                 "quantity": 4,
                 "price": "0.25",
                 "subtotal": "1",
                 "location": "brave.com",
                 "description": ""
               }
              ]
            })";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUOrderItem item;
  item.quantity = 4;
  item.sku = "asdfasfasfdsdf";
  item.type = mojom::SKUOrderItemType::SINGLE_USE;
  std::vector<mojom::SKUOrderItem> items;
  items.push_back(item);

  MockFunction<PostOrderCallback> callback;
  EXPECT_CALL(callback, Call)
      .Times(1)
      .WillOnce([](mojom::Result result, mojom::SKUOrderPtr order) {
        auto expected_order_item = mojom::SKUOrderItem::New();
        expected_order_item->order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
        expected_order_item->sku = "asdfasfasfdsdf";
        expected_order_item->type = mojom::SKUOrderItemType::SINGLE_USE;
        expected_order_item->order_item_id =
            "9c9aed7f-b349-452e-80a8-95faf2b1600d";
        expected_order_item->quantity = 4;
        expected_order_item->price = 0.25;

        mojom::SKUOrder expected_order;
        expected_order.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
        expected_order.total_amount = 1;
        expected_order.location = "brave.com";
        expected_order.status = mojom::SKUOrderStatus::PENDING;
        expected_order.items.push_back(std::move(expected_order_item));

        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_TRUE(expected_order.Equals(*order));
      });
  order_.Request(items, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostOrderTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUOrderItem item;
  item.quantity = 4;
  item.sku = "asdfasfasfdsdf";
  item.type = mojom::SKUOrderItemType::SINGLE_USE;
  std::vector<mojom::SKUOrderItem> items;
  items.push_back(item);

  MockFunction<PostOrderCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::RETRY_SHORT, IsFalse())).Times(1);
  order_.Request(items, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostOrderTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUOrderItem item;
  item.quantity = 4;
  item.sku = "asdfasfasfdsdf";
  item.type = mojom::SKUOrderItemType::SINGLE_USE;
  std::vector<mojom::SKUOrderItem> items;
  items.push_back(item);

  MockFunction<PostOrderCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::RETRY_SHORT, IsFalse())).Times(1);
  order_.Request(items, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostOrderTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::SKUOrderItem item;
  item.quantity = 4;
  item.sku = "asdfasfasfdsdf";
  item.type = mojom::SKUOrderItemType::SINGLE_USE;
  std::vector<mojom::SKUOrderItem> items;
  items.push_back(item);

  MockFunction<PostOrderCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED, IsFalse())).Times(1);
  order_.Request(items, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
