/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_order/post_order.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsPostOrderTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().rewards_payment_url().Resolve(
            "/v1/orders");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::POST, std::move(response));

    endpoint::payment::PostOrder endpoint(engine());

    return WaitForValues<mojom::Result, mojom::SKUOrderPtr>([&](auto callback) {
      mojom::SKUOrderItem item;
      item.quantity = 4;
      item.sku = "asdfasfasfdsdf";
      item.type = mojom::SKUOrderItemType::SINGLE_USE;

      endpoint.Request({item}, std::move(callback));
    });
  }
};

TEST_F(RewardsPostOrderTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 201;
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

  auto [result, order] = Request(std::move(response));

  auto expected_order_item = mojom::SKUOrderItem::New();
  expected_order_item->order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  expected_order_item->sku = "asdfasfasfdsdf";
  expected_order_item->type = mojom::SKUOrderItemType::SINGLE_USE;
  expected_order_item->order_item_id = "9c9aed7f-b349-452e-80a8-95faf2b1600d";
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
}

TEST_F(RewardsPostOrderTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 400;

  auto [result, order] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
  EXPECT_FALSE(order);
}

TEST_F(RewardsPostOrderTest, ServerError500) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;

  auto [result, order] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
  EXPECT_FALSE(order);
}

TEST_F(RewardsPostOrderTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, order] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_FALSE(order);
}

}  // namespace brave_rewards::internal
