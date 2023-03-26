/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_order/post_order.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostOrderTest.*

using ::testing::_;

namespace ledger {
namespace endpoint {
namespace payment {

class PostOrderTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostOrder order_{&mock_ledger_impl_};
};

TEST_F(PostOrderTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
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

  order_.Request(items, [](const mojom::Result result,
                           mojom::SKUOrderPtr order) {
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

    EXPECT_EQ(result, mojom::Result::LEDGER_OK);
    EXPECT_TRUE(expected_order.Equals(*order));
  });
}

TEST_F(PostOrderTest, ServerError400) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
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

  order_.Request(items,
                 [](const mojom::Result result, mojom::SKUOrderPtr order) {
                   EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
                   EXPECT_TRUE(!order);
                 });
}

TEST_F(PostOrderTest, ServerError500) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
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

  order_.Request(items,
                 [](const mojom::Result result, mojom::SKUOrderPtr order) {
                   EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
                   EXPECT_TRUE(!order);
                 });
}

TEST_F(PostOrderTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
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

  order_.Request(items,
                 [](const mojom::Result result, mojom::SKUOrderPtr order) {
                   EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                   EXPECT_TRUE(!order);
                 });
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
