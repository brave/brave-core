/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_order_endpoint.h"

#include <utility>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "net/http/http_status_code.h"

namespace ledger {

class PostOrderEndpointTest : public BATLedgerTest {
 protected:
  PostOrderEndpoint& GetEndpoint() {
    return context().Get<PostOrderEndpoint>();
  }
};

TEST_F(PostOrderEndpointTest, MapRequest) {
  URLRequest request = GetEndpoint().MapRequest({{"user-vote-sku", 5}});

  auto& req = request.req();
  EXPECT_EQ(req.method, mojom::UrlMethod::POST);
  EXPECT_EQ(req.url, "https://payment.rewards.brave.com/v1/orders");
  EXPECT_EQ(req.content, R"({"items":[{"quantity":5,"sku":"user-vote-sku"}]})");
}

TEST_F(PostOrderEndpointTest, MapResponseSuccess) {
  auto resp = mojom::UrlResponse::New();
  resp->status_code = net::HTTP_OK;
  resp->body = R"json(
      {
        "id": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
        "totalPrice": "1",
        "status": "pending",
        "items": [
        {
          "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
          "sku": "user-wallet-vote",
          "quantity": 4,
          "price": "0.25"
        }
      ]
    }
  )json";

  absl::optional<PaymentOrder> result =
      GetEndpoint().MapResponse(URLResponse(std::move(resp)));

  ASSERT_TRUE(result);
  EXPECT_EQ(result->id, "f2e6494e-fb21-44d1-90e9-b5408799acd8");
  EXPECT_EQ(result->total_price, 1.0);
  EXPECT_EQ(result->status, PaymentOrderStatus::kPending);
  ASSERT_EQ(result->items.size(), size_t(1));
  EXPECT_EQ(result->items[0].id, "9c9aed7f-b349-452e-80a8-95faf2b1600d");
  EXPECT_EQ(result->items[0].sku, "user-wallet-vote");
  EXPECT_EQ(result->items[0].quantity, 4);
  EXPECT_EQ(result->items[0].price, 0.25);
}

}  // namespace ledger
